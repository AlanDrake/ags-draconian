//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// Field tables: one row per data member of a struct, so that code which has to treat every field alike
// (comparing two objects, printing one, checking that a save restores all of it) is written once and cannot
// miss a field.
//
// C++ has no reflection, so the struct gets a table in its source file:
//
//   constexpr FieldRow<CharacterDef> kCharacterDefFields[] = {
//       Field<&CharacterDef::Baseline>("Baseline"),
//       Field<&CharacterDef::Name>("Name"),
//   };
//   static_assert(FieldsCoverStruct(kCharacterDefFields), "CharacterDef: a field has no row");
//   FieldTable<CharacterDef> CharacterDef::Fields() { return kCharacterDefFields; }
//
// A row says what a field is: its name, where it is in the object, the operations of its type and its role in
// the game's state. It never says what happens when the field changes; that stays in the code of the struct.
// The default of a field is its member initializer.
//
// The static_assert counts the struct's members at compile time, so a field added without a row does not
// build. The count works for an aggregate (no private or protected data, no constructor written out, no
// virtual function); a base class is one row, BaseFields.
//
// A member's type needs its operations (FieldValue below): numbers, enums, String, points, rectangles, time points,
// vectors, maps, sets and arrays (plain and std::array) of those have them, and so has every struct with a table of
// its own.
//
//=============================================================================
#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "util/geometry.h"
#include "util/string.h"

namespace AGS
{
namespace Common
{

// What a field is to the state of the game
enum class FieldRole : uint8_t
{
    Saved,          // a part of the state: a restored game has the value the saved one had
    FromDefinition, // the definition gives it again whenever the object is loaded (a room's entity: at every load of
                    // the room); play may change it meanwhile, a save does not hold it
    Derived,        // computed from other fields; a restore computes it again
    Opaque,         // not a value to compare or store (a cache, a pointer); listed so that the table is whole
    Transient       // what the running game is doing now (a blocking wait, an overlay on screen, a fade): a save
                    // does not hold it, a restore leaves the running game's value or sets one of its own; compared
                    // as a saved one
};

// The operations on a value of one type; every row whose member has the type shares them
struct FieldOps
{
    bool (*Equal)(const void *a, const void *b);
    String (*Print)(const void *value);
    // Changes the value to a different valid one: the tests that check a round trip set every field with it. A struct
    // with a table of its own changes its fields of the role (AlterFields); another type ignores the role.
    void (*Alter)(void *value, FieldRole role);
    // For a struct with a table of its own, null for any other type: adds a line per field in which the two
    // values differ (DiffFields), named "<prefix><Member>.<field>", or "<prefix><field>" for a base class
    void (*Diff)(const void *a, const void *b, bool with_derived, const String &prefix, const char *member,
                 std::vector<String> &lines);
};

// One row of a table
template <typename T>
struct FieldRow
{
    const char *Name;
    const void *(*Address)(const T &object);
    const FieldOps *Ops; // null for an opaque field
    FieldRole Role;
    uint16_t Initializers; // how many initializers the member takes in the struct's aggregate initialization
};

// The rows of a struct
template <typename T>
struct FieldTable
{
    const FieldRow<T> *Rows = nullptr;
    size_t Count = 0;

    constexpr FieldTable() = default;
    template <size_t N>
    constexpr FieldTable(const FieldRow<T> (&rows)[N]) : Rows(rows),
                                                         Count(N)
    {
    }

    constexpr const FieldRow<T> *begin() const { return Rows; }
    constexpr const FieldRow<T> *end() const { return Rows + Count; }
};

template <typename V, typename = void>
struct FieldValue;

namespace FieldDetail
{

template <typename M>
struct MemberPointer;
template <typename C, typename V>
struct MemberPointer<V C::*>
{
    using Class = C;
    using Value = V;
};

template <auto Member>
const void *AddressOf(const typename MemberPointer<decltype(Member)>::Class &object)
{
    return &(object.*Member);
}

template <typename T, typename Base>
const void *AddressOfBase(const T &object)
{
    return static_cast<const Base *>(&object);
}

template <typename V>
bool Equal(const void *a, const void *b)
{
    return FieldValue<V>::Equal(*static_cast<const V *>(a), *static_cast<const V *>(b));
}

template <typename V>
String Print(const void *value)
{
    return FieldValue<V>::Print(*static_cast<const V *>(value));
}

template <typename V, typename = void>
struct HasFields : std::false_type
{
};
template <typename V>
struct HasFields<V, std::void_t<decltype(V::Fields())>> : std::true_type
{
};

template <typename T>
void DiffInto(const T &a, const T &b, bool with_derived, const String &prefix, std::vector<String> &lines);
template <typename T>
void AlterInto(T &object, FieldRole role);

template <typename V>
void Alter(void *value, FieldRole role)
{
    if constexpr (HasFields<V>::value)
        AlterInto(*static_cast<V *>(value), role);
    else
        FieldValue<V>::Alter(*static_cast<V *>(value));
}

template <typename V, bool IsBase>
void Diff(const void *a, const void *b, bool with_derived, const String &prefix, const char *member,
          std::vector<String> &lines)
{
    DiffInto(*static_cast<const V *>(a), *static_cast<const V *>(b), with_derived,
             IsBase ? prefix : String::FromFormat("%s%s.", prefix.GetCStr(), member), lines);
}

template <typename V, bool IsBase>
constexpr decltype(FieldOps::Diff) DiffOp()
{
    if constexpr (HasFields<V>::value)
        return &Diff<V, IsBase>;
    else
        return nullptr;
}

template <typename V>
inline constexpr FieldOps kOps = {&Equal<V>, &Print<V>, &Alter<V>, DiffOp<V, false>()};
// A base class's fields are named as the struct's own
template <typename V>
inline constexpr FieldOps kBaseOps = {&Equal<V>, &Print<V>, &Alter<V>, DiffOp<V, true>()};

// The initializers a member of type V takes when the struct is initialized as an aggregate: one, or one per
// element of an array (braces are left out for an array, so each element takes its own)
template <typename V>
struct InitializerCount : std::integral_constant<size_t, 1>
{
};
template <typename V, size_t N>
struct InitializerCount<V[N]> : std::integral_constant<size_t, N * InitializerCount<V>::value>
{
};

// A value that converts to any type, to try an aggregate initialization with a number of initializers
struct AnyValue
{
    size_t Index;
    template <typename V>
    constexpr operator V() const noexcept;
};

template <typename T, typename Sequence, typename = void>
struct TakesInitializers : std::false_type
{
};
template <typename T, size_t... I>
struct TakesInitializers<T, std::index_sequence<I...>, std::void_t<decltype(T{AnyValue{I}...})>> : std::true_type
{
};

// The largest count of initializers T takes, between low (which it takes) and high (which it does not)
template <typename T, size_t Low, size_t High>
constexpr size_t MaxInitializers()
{
    if constexpr (High - Low <= 1)
    {
        return Low;
    }
    else if constexpr (TakesInitializers<T, std::make_index_sequence<(Low + High) / 2>>::value)
    {
        return MaxInitializers<T, (Low + High) / 2, High>();
    }
    else
    {
        return MaxInitializers<T, Low, (Low + High) / 2>();
    }
}

} // namespace FieldDetail

// A row for a member: Field<&CharacterDef::Baseline>("Baseline")
template <auto Member>
constexpr auto Field(const char *name, FieldRole role = FieldRole::Saved)
{
    using Pointer = FieldDetail::MemberPointer<decltype(Member)>;
    return FieldRow<typename Pointer::Class>{
        name, &FieldDetail::AddressOf<Member>, &FieldDetail::kOps<typename Pointer::Value>, role,
        static_cast<uint16_t>(FieldDetail::InitializerCount<typename Pointer::Value>::value)};
}

// A row for a member that is not a value (FieldRole::Opaque); its type needs no operations
template <auto Member>
constexpr auto OpaqueField(const char *name)
{
    using Pointer = FieldDetail::MemberPointer<decltype(Member)>;
    return FieldRow<typename Pointer::Class>{
        name, &FieldDetail::AddressOf<Member>, nullptr, FieldRole::Opaque,
        static_cast<uint16_t>(FieldDetail::InitializerCount<typename Pointer::Value>::value)};
}

// A row for a base class of T, which has a table of its own: BaseFields<ViewAnimateParams, AnimFlowParams>("Flow")
template <typename T, typename Base>
constexpr FieldRow<T> BaseFields(const char *name)
{
    static_assert(std::is_base_of_v<Base, T>, "BaseFields names a base class of the struct");
    return FieldRow<T>{name, &FieldDetail::AddressOfBase<T, Base>, &FieldDetail::kBaseOps<Base>, FieldRole::Saved, 1};
}

// Tells whether the rows take as many initializers as the aggregate T has: with one row per member, no member
// is left out. It does not tell two rows of one member from two members; the table's test does
// (FieldsAreDistinct).
template <typename T, size_t N>
constexpr bool FieldsCoverStruct(const FieldRow<T> (&rows)[N])
{
    static_assert(std::is_aggregate_v<T>, "the fields of a struct can be counted only if it is an aggregate");
    size_t initializers = 0;
    for (const FieldRow<T> &row : rows)
        initializers += row.Initializers;
    return initializers == FieldDetail::MaxInitializers<T, 0, sizeof(T) + 1>();
}

// Tells whether no two rows have the same name or the same member
template <typename T>
bool FieldsAreDistinct(const T &object, FieldTable<T> table = T::Fields())
{
    for (const FieldRow<T> *a = table.begin(); a != table.end(); ++a)
    {
        for (const FieldRow<T> *b = a + 1; b != table.end(); ++b)
        {
            if (a->Address(object) == b->Address(object) || String(a->Name) == b->Name)
                return false;
        }
    }
    return true;
}

template <typename T>
void FieldDetail::DiffInto(const T &a, const T &b, bool with_derived, const String &prefix, std::vector<String> &lines)
{
    for (const FieldRow<T> &row : T::Fields())
    {
        if (row.Role == FieldRole::Opaque || (row.Role == FieldRole::Derived && !with_derived))
            continue;
        const void *in_a = row.Address(a);
        const void *in_b = row.Address(b);
        if (row.Ops->Diff)
            row.Ops->Diff(in_a, in_b, with_derived, prefix, row.Name, lines);
        else if (!row.Ops->Equal(in_a, in_b))
            lines.push_back(String::FromFormat("%s%s: %s != %s", prefix.GetCStr(), row.Name,
                                               row.Ops->Print(in_a).GetCStr(), row.Ops->Print(in_b).GetCStr()));
    }
}

// Lists the fields in which two objects differ, one line each: "Baseline: -1 != 120". Opaque fields are not
// compared; the derived ones are unless with_derived is false; the others always. A base or a member whose type has a table is
// compared field by field with the same rule: "TintRed: 0 != 1" for a field of a base, "Anim.Delay: 0 != 1"
// for a field of a member.
template <typename T>
std::vector<String> DiffFields(const T &a, const T &b, bool with_derived = true)
{
    std::vector<String> lines;
    FieldDetail::DiffInto(a, b, with_derived, String(), lines);
    return lines;
}

// Prints the fields of an object: "{Baseline: -1, Name: "Roger"}"
template <typename T>
String PrintFields(const T &object)
{
    String text = "{";
    for (const FieldRow<T> &row : T::Fields())
    {
        if (row.Role == FieldRole::Opaque)
            continue;
        if (text.GetLength() > 1)
            text.Append(", ");
        text.AppendFmt("%s: %s", row.Name, row.Ops->Print(row.Address(object)).GetCStr());
    }
    text.AppendChar('}');
    return text;
}

template <typename T>
void FieldDetail::AlterInto(T &object, FieldRole role)
{
    for (const FieldRow<T> &row : T::Fields())
    {
        // a base or a member with a table, as a saved row, has the roles of its own rows
        if (row.Ops && (row.Role == role || (row.Role == FieldRole::Saved && row.Ops->Diff)))
            row.Ops->Alter(const_cast<void *>(row.Address(object)), role);
    }
}

// Changes every field of a role in an object to another value, for a test that checks a round trip: the saved fields,
// or with FieldRole::FromDefinition the fields that a load must give back from the definition. A base or a member
// whose type has a table is changed field by field with the same role.
template <typename T>
void AlterFields(T &object, FieldRole role = FieldRole::Saved)
{
    FieldDetail::AlterInto(object, role);
}

// The operations on a value of type V. The primary template is for a struct that has a table (V::Fields());
// the specializations below are for the plain types. Another type gets a specialization beside its definition.
template <typename V, typename>
struct FieldValue
{
    static_assert(FieldDetail::HasFields<V>::value,
                  "a field of this type needs a FieldValue specialization, or the type a table (Fields())");
    static bool Equal(const V &a, const V &b) { return DiffFields(a, b).empty(); }
    static String Print(const V &value) { return PrintFields(value); }
    // Changes the saved fields: an element of a vector, an array or a map has no role but the container's row's (a
    // member or a base with a table gets the role of AlterFields through FieldOps::Alter instead)
    static void Alter(V &value) { AlterFields(value); }
};

// Integers, floating-point numbers, bools and enums
template <typename V>
struct FieldValue<V, std::enable_if_t<std::is_arithmetic_v<V> || std::is_enum_v<V>>>
{
    static bool Equal(const V &a, const V &b) { return a == b; }
    static String Print(const V &value)
    {
        if constexpr (std::is_floating_point_v<V>)
            return String::FromFormat("%g", static_cast<double>(value));
        else
            return String::FromFormat("%lld", static_cast<long long>(value));
    }
    // Adds one, or takes one from the largest value of an integer type (so that it does not overflow)
    static void Alter(V &value)
    {
        if constexpr (std::is_same_v<V, bool>)
            value = !value;
        else if constexpr (std::is_integral_v<V>)
            value = (value == std::numeric_limits<V>::max()) ? static_cast<V>(value - 1) : static_cast<V>(value + 1);
        else
            value = static_cast<V>(static_cast<std::conditional_t<std::is_enum_v<V>, int, V>>(value) + 1);
    }
};

template <>
struct FieldValue<String>
{
    static bool Equal(const String &a, const String &b) { return a == b; }
    static String Print(const String &value) { return String::FromFormat("\"%s\"", value.GetCStr()); }
    static void Alter(String &value) { value.AppendChar('~'); }
};

template <typename V>
struct FieldValue<PointT<V>>
{
    static bool Equal(const PointT<V> &a, const PointT<V> &b) { return a.X == b.X && a.Y == b.Y; }
    static String Print(const PointT<V> &value)
    {
        return String::FromFormat("(%s, %s)", FieldValue<V>::Print(value.X).GetCStr(),
                                  FieldValue<V>::Print(value.Y).GetCStr());
    }
    static void Alter(PointT<V> &value)
    {
        FieldValue<V>::Alter(value.X);
        FieldValue<V>::Alter(value.Y);
    }
};

// Alter moves the rectangle and makes it one pixel wider and higher
template <>
struct FieldValue<Rect>
{
    static bool Equal(const Rect &a, const Rect &b) { return a == b; }
    static String Print(const Rect &value)
    {
        return String::FromFormat("(%d, %d)-(%d, %d)", value.Left, value.Top, value.Right, value.Bottom);
    }
    static void Alter(Rect &value) { value = Rect(value.Left + 1, value.Top + 1, value.Right + 2, value.Bottom + 2); }
};

namespace FieldDetail
{

// "[a, b]": the items of a sequence, each printed with the operations of its type
template <typename V, typename Iterator>
String PrintItems(Iterator begin, Iterator end)
{
    String text = "[";
    for (Iterator item = begin; item != end; ++item)
    {
        if (item != begin)
            text.Append(", ");
        text.Append(FieldValue<V>::Print(*item));
    }
    text.AppendChar(']');
    return text;
}

// The operations of an array of a fixed size (a plain one or a std::array): element by element
template <typename Array, typename V>
struct ArrayValue
{
    static bool Equal(const Array &a, const Array &b)
    {
        return std::equal(std::begin(a), std::end(a), std::begin(b), &FieldValue<V>::Equal);
    }
    static String Print(const Array &value) { return PrintItems<V>(std::begin(value), std::end(value)); }
    static void Alter(Array &value)
    {
        for (V &item : value)
            FieldValue<V>::Alter(item);
    }
};

} // namespace FieldDetail

template <typename V>
struct FieldValue<std::vector<V>>
{
    static bool Equal(const std::vector<V> &a, const std::vector<V> &b)
    {
        return std::equal(a.begin(), a.end(), b.begin(), b.end(), &FieldValue<V>::Equal);
    }
    static String Print(const std::vector<V> &value) { return FieldDetail::PrintItems<V>(value.begin(), value.end()); }
    static void Alter(std::vector<V> &value)
    {
        value.emplace_back();
        FieldValue<V>::Alter(value.back());
    }
};

template <typename V, size_t N>
struct FieldValue<V[N]> : FieldDetail::ArrayValue<V[N], V>
{
};

template <typename V, size_t N>
struct FieldValue<std::array<V, N>> : FieldDetail::ArrayValue<std::array<V, N>, V>
{
};

// A map prints in the order of its keys, so that two equal maps print alike; a map with a hash and a comparison of
// its own (StringIMap) finds the keys with them
template <typename K, typename V, typename Hash, typename KeyEqual, typename Allocator>
struct FieldValue<std::unordered_map<K, V, Hash, KeyEqual, Allocator>>
{
    using Map = std::unordered_map<K, V, Hash, KeyEqual, Allocator>;
    static bool Equal(const Map &a, const Map &b)
    {
        if (a.size() != b.size())
            return false;
        for (const auto &item : a)
        {
            const auto in_b = b.find(item.first);
            if (in_b == b.end() || !FieldValue<V>::Equal(item.second, in_b->second))
                return false;
        }
        return true;
    }
    static String Print(const Map &value)
    {
        std::vector<K> keys;
        for (const auto &item : value)
            keys.push_back(item.first);
        std::sort(keys.begin(), keys.end());
        String text = "{";
        for (const K &key : keys)
        {
            if (text.GetLength() > 1)
                text.Append(", ");
            text.AppendFmt("%s: %s", FieldValue<K>::Print(key).GetCStr(),
                           FieldValue<V>::Print(value.at(key)).GetCStr());
        }
        text.AppendChar('}');
        return text;
    }
    static void Alter(Map &value)
    {
        K key{};
        FieldValue<K>::Alter(key);
        FieldValue<V>::Alter(value[key]);
    }
};

// A set prints in the order of its keys, as a map; Alter adds a key it does not have
template <typename K, typename Hash, typename KeyEqual, typename Allocator>
struct FieldValue<std::unordered_set<K, Hash, KeyEqual, Allocator>>
{
    using Set = std::unordered_set<K, Hash, KeyEqual, Allocator>;
    static bool Equal(const Set &a, const Set &b)
    {
        if (a.size() != b.size())
            return false;
        for (const K &key : a)
        {
            if (b.find(key) == b.end())
                return false;
        }
        return true;
    }
    static String Print(const Set &value)
    {
        std::vector<K> keys(value.begin(), value.end());
        std::sort(keys.begin(), keys.end());
        return FieldDetail::PrintItems<K>(keys.begin(), keys.end());
    }
    static void Alter(Set &value)
    {
        K key{};
        do
            FieldValue<K>::Alter(key);
        while (value.find(key) != value.end());
        value.insert(key);
    }
};

// A point in time prints as the count of its clock's ticks; Alter moves it one tick on
template <typename Clock, typename Duration>
struct FieldValue<std::chrono::time_point<Clock, Duration>>
{
    using TimePoint = std::chrono::time_point<Clock, Duration>;
    static bool Equal(const TimePoint &a, const TimePoint &b) { return a == b; }
    static String Print(const TimePoint &value)
    {
        return String::FromFormat("%lld", static_cast<long long>(value.time_since_epoch().count()));
    }
    static void Alter(TimePoint &value) { value += Duration(1); }
};

} // namespace Common
} // namespace AGS
