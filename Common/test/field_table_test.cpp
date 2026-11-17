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
#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "game/audio_clip_def.h"
#include "game/audio_clip_type_def.h"
#include "game/character_def.h"
#include "game/dialog_def.h"
#include "game/game_struct_defines.h"
#include "game/inventory_item_def.h"
#include "game/mouse_cursor_def.h"
#include "game/view_def.h"
#include "gtest/gtest.h"
#include "util/field_table.h"
#include "util/geometry.h"
#include "util/string.h"
#include "util/string_types.h"

using namespace AGS::Common;

namespace
{

enum Paint
{
    kPaint_Red,
    kPaint_Green
};

struct Part
{
    int Count = 1;
    float Ratio = 2.f;

    static FieldTable<Part> Fields();
};

struct Kind
{
    int16_t Flow = 0;

    static FieldTable<Kind> Fields();
};

// A struct with a member of every sort the tables know
struct Sample : Kind
{
    int Number = 0;
    int16_t Small = -1;
    char Letter = 0;
    bool Flag = false;
    Paint Colour = kPaint_Red;
    String Name = {};
    Pointf Anchor = Pointf(0.5f, 1.f);
    Point Offset;
    std::vector<int> List;
    std::unordered_map<int, int> Map;
    int Fixed[3] = {};
    Point Corners[2];
    Part Nested;
    const char *Cache = nullptr;

    static FieldTable<Sample> Fields();
};

// A part with a derived field, as the size of the sprite in the graphic part of an entity's state, and a field that the
// definition gives again at every load, as a room entity's script name
struct Look
{
    int Tint = 0;
    int Width = 0;
    int Room = 0;

    static FieldTable<Look> Fields();
};

// A struct with the part as a base and as a member
struct Entity : Look
{
    int Number = 0;
    Look Shadow;

    static FieldTable<Entity> Fields();
};

// A struct with the members of the game's state that the other samples do not have: a std::array, a set, a point in
// time, a number at the largest value of its type, and a transient field (a blocking wait's result)
struct Session
{
    std::array<bool, 2> Switches = {{false, true}};
    std::unordered_set<String> Tokens;
    std::chrono::steady_clock::time_point Until{};
    int ZOrder = INT32_MAX;
    int SkippedBy = 0;

    static FieldTable<Session> Fields();
};

// Lists of structs with tables, as a view's loops and their frames: a frame with a saved field and one from the
// definition, a strip whose count is the definition's and whose frames are a list
struct Cel
{
    int Delay = 0;
    int Reserved = 0;

    static FieldTable<Cel> Fields();
};

struct Strip
{
    int CelCount = 0;
    std::vector<Cel> Cels;

    static FieldTable<Strip> Fields();
};

// The lists of every kind, a list of plain values, and a member and a list whose rows say FromDefinition, which covers
// the whole of each: every field in it, whatever its own row says
struct Reel
{
    std::vector<Strip> Strips;
    Cel Pair[2];
    std::array<Cel, 2> Ends;
    std::vector<int> Numbers;
    Part Template;
    std::vector<Look> Skies;

    static FieldTable<Reel> Fields();
};

constexpr FieldRow<Part> kPartFields[] = {
    Field<&Part::Count>("Count"),
    Field<&Part::Ratio>("Ratio"),
};
static_assert(FieldsCoverStruct(kPartFields), "Part: a field has no row");

constexpr FieldRow<Kind> kKindFields[] = {
    Field<&Kind::Flow>("Flow"),
};
static_assert(FieldsCoverStruct(kKindFields), "Kind: a field has no row");

constexpr FieldRow<Sample> kSampleFields[] = {
    BaseFields<Sample, Kind>("Kind"),
    Field<&Sample::Number>("Number"),
    Field<&Sample::Small>("Small"),
    Field<&Sample::Letter>("Letter"),
    Field<&Sample::Flag>("Flag"),
    Field<&Sample::Colour>("Colour"),
    Field<&Sample::Name>("Name"),
    Field<&Sample::Anchor>("Anchor"),
    Field<&Sample::Offset>("Offset", FieldRole::Derived),
    Field<&Sample::List>("List"),
    Field<&Sample::Map>("Map"),
    Field<&Sample::Fixed>("Fixed"),
    Field<&Sample::Corners>("Corners"),
    Field<&Sample::Nested>("Nested"),
    OpaqueField<&Sample::Cache>("Cache"),
};
// The base is one initializer, the arrays three and two: a table without one of the rows does not build
static_assert(FieldsCoverStruct(kSampleFields), "Sample: a field has no row");

constexpr FieldRow<Look> kLookFields[] = {
    Field<&Look::Tint>("Tint"),
    Field<&Look::Width>("Width", FieldRole::Derived),
    Field<&Look::Room>("Room", FieldRole::FromDefinition),
};
static_assert(FieldsCoverStruct(kLookFields), "Look: a field has no row");

constexpr FieldRow<Entity> kEntityFields[] = {
    BaseFields<Entity, Look>("Look"),
    Field<&Entity::Number>("Number"),
    Field<&Entity::Shadow>("Shadow"),
};
static_assert(FieldsCoverStruct(kEntityFields), "Entity: a field has no row");

constexpr FieldRow<Session> kSessionFields[] = {
    Field<&Session::Switches>("Switches"),
    Field<&Session::Tokens>("Tokens"),
    Field<&Session::Until>("Until"),
    Field<&Session::ZOrder>("ZOrder"),
    Field<&Session::SkippedBy>("SkippedBy", FieldRole::Transient),
};
// A std::array is one initializer, as a struct
static_assert(FieldsCoverStruct(kSessionFields), "Session: a field has no row");

constexpr FieldRow<Cel> kCelFields[] = {
    Field<&Cel::Delay>("Delay"),
    Field<&Cel::Reserved>("Reserved", FieldRole::FromDefinition),
};
static_assert(FieldsCoverStruct(kCelFields), "Cel: a field has no row");

constexpr FieldRow<Strip> kStripFields[] = {
    Field<&Strip::CelCount>("CelCount", FieldRole::FromDefinition),
    Field<&Strip::Cels>("Cels"),
};
static_assert(FieldsCoverStruct(kStripFields), "Strip: a field has no row");

constexpr FieldRow<Reel> kReelFields[] = {
    Field<&Reel::Strips>("Strips"),
    Field<&Reel::Pair>("Pair"),
    Field<&Reel::Ends>("Ends"),
    Field<&Reel::Numbers>("Numbers"),
    Field<&Reel::Template>("Template", FieldRole::FromDefinition),
    Field<&Reel::Skies>("Skies", FieldRole::FromDefinition),
};
static_assert(FieldsCoverStruct(kReelFields), "Reel: a field has no row");

// The same rows without the last one leave a member out
constexpr FieldRow<Part> kPartFieldsShort[] = {
    Field<&Part::Count>("Count"),
};
static_assert(!FieldsCoverStruct(kPartFieldsShort), "a missing row must be found");

FieldTable<Part> Part::Fields()
{
    return kPartFields;
}

FieldTable<Kind> Kind::Fields()
{
    return kKindFields;
}

FieldTable<Sample> Sample::Fields()
{
    return kSampleFields;
}

FieldTable<Look> Look::Fields()
{
    return kLookFields;
}

FieldTable<Entity> Entity::Fields()
{
    return kEntityFields;
}

FieldTable<Session> Session::Fields()
{
    return kSessionFields;
}

FieldTable<Cel> Cel::Fields()
{
    return kCelFields;
}

FieldTable<Strip> Strip::Fields()
{
    return kStripFields;
}

FieldTable<Reel> Reel::Fields()
{
    return kReelFields;
}

// A reel with two strips of two frames each, and two skies
Reel MakeReel()
{
    Reel reel;
    reel.Strips.resize(2);
    for (Strip &strip : reel.Strips)
    {
        strip.CelCount = 2;
        strip.Cels.resize(2);
    }
    reel.Skies.resize(2);
    return reel;
}

std::string Lines(const std::vector<String> &lines)
{
    std::string text;
    for (const String &line : lines)
        text += std::string(line.GetCStr()) + "\n";
    return text;
}

// The rows of a table that have a role
template <typename T>
size_t RowsOfRole(FieldRole role)
{
    size_t count = 0;
    for (const FieldRow<T> &row : T::Fields())
        count += (row.Role == role) ? 1 : 0;
    return count;
}

// The rows of a table that have a role and whose member is one value: not a struct with a table nor a list of such
template <typename T>
size_t ValueRowsOfRole(FieldRole role)
{
    size_t count = 0;
    for (const FieldRow<T> &row : T::Fields())
        count += (row.Role == role && row.Ops && !row.Ops->Diff) ? 1 : 0;
    return count;
}

// A definition's table is whole: it has the rows of each role that the test expects, its rows are distinct, two new
// objects do not differ, and AlterFields with a role changes a field per value row of that role and no other (a list's
// row of plain values gets an element more; a list of structs with tables is empty in a new object, so its row gives
// no line, and no row of these tables is a struct with a table of its own)
template <typename T>
void ExpectWholeTable(size_t saved, size_t from_definition, size_t opaque)
{
    EXPECT_EQ(saved, RowsOfRole<T>(FieldRole::Saved));
    EXPECT_EQ(from_definition, RowsOfRole<T>(FieldRole::FromDefinition));
    EXPECT_EQ(0u, RowsOfRole<T>(FieldRole::Derived));
    EXPECT_EQ(opaque, RowsOfRole<T>(FieldRole::Opaque));
    const T a{};
    const T b{};
    EXPECT_TRUE(FieldsAreDistinct(a));
    EXPECT_EQ("", Lines(DiffFields(a, b)));
    for (const FieldRole role : {FieldRole::Saved, FieldRole::FromDefinition})
    {
        SCOPED_TRACE(testing::Message() << "role " << static_cast<int>(role));
        T altered{};
        AlterFields(altered, role);
        EXPECT_EQ(ValueRowsOfRole<T>(role), DiffFields(a, altered).size()) << Lines(DiffFields(a, altered));
    }
}

} // namespace

TEST(FieldTable, EqualObjectsHaveNoDifference)
{
    Sample a;
    Sample b;
    EXPECT_EQ("", Lines(DiffFields(a, b)));
    EXPECT_TRUE(FieldsAreDistinct(a));
}

TEST(FieldTable, DiffNamesTheFieldAndBothValues)
{
    Sample a;
    Sample b;
    b.Small = 7;
    b.Name = "Roger";
    b.Nested.Ratio = 0.5f;
    b.Flow = 3;
    EXPECT_EQ("Flow: 0 != 3\n"
              "Small: -1 != 7\n"
              "Name: \"\" != \"Roger\"\n"
              "Nested.Ratio: 2 != 0.5\n",
              Lines(DiffFields(a, b)));
}

// A base or a member whose type has a table is compared field by field: a line names the field, with the member's
// name before it (a base's fields are named as the struct's own), and a derived field of the part is left out
// with the struct's own
TEST(FieldTable, DiffDescendsIntoAStructWithATable)
{
    Entity a;
    Entity b;
    b.Tint = 1;
    b.Shadow.Tint = 2;
    EXPECT_EQ("Tint: 0 != 1\n"
              "Shadow.Tint: 0 != 2\n",
              Lines(DiffFields(a, b, false)));

    Entity c;
    c.Width = 4;
    c.Shadow.Width = 5;
    EXPECT_EQ("", Lines(DiffFields(a, c, false)));
    EXPECT_EQ("Width: 0 != 4\n"
              "Shadow.Width: 0 != 5\n",
              Lines(DiffFields(a, c)));
}

// A field that the definition gives again is compared as a saved one, also without the derived fields, and in a base
// and in a member with a table
TEST(FieldTable, FieldsFromTheDefinitionAreCompared)
{
    Entity a;
    Entity b;
    b.Room = 1;
    b.Shadow.Room = 2;
    const std::string lines = "Room: 0 != 1\n"
                              "Shadow.Room: 0 != 2\n";
    EXPECT_EQ(lines, Lines(DiffFields(a, b)));
    EXPECT_EQ(lines, Lines(DiffFields(a, b, false)));
}

// AlterFields changes the saved fields unless it is given another role: then the fields of that role, and no other; the
// role passes down into a base and a member with a table
TEST(FieldTable, AlterChangesTheFieldsOfOneRole)
{
    Entity a;
    Entity saved;
    AlterFields(saved);
    EXPECT_EQ("Tint: 0 != 1\n"
              "Number: 0 != 1\n"
              "Shadow.Tint: 0 != 1\n",
              Lines(DiffFields(a, saved)));

    Entity from_definition;
    AlterFields(from_definition, FieldRole::FromDefinition);
    EXPECT_EQ("Room: 0 != 1\n"
              "Shadow.Room: 0 != 1\n",
              Lines(DiffFields(a, from_definition)));
}

TEST(FieldTable, PrintsTheFieldsOfEveryRoleButOpaque)
{
    Look look;
    look.Room = 3;
    EXPECT_STREQ("{Tint: 0, Width: 0, Room: 3}", PrintFields(look).GetCStr());
}

TEST(FieldTable, AlterChangesEverySavedFieldAndNoOther)
{
    Sample a;
    Sample b;
    AlterFields(b);
    // 15 rows, less the derived one and the opaque one; the nested struct gives a line per field (two)
    EXPECT_EQ(14u, DiffFields(a, b).size()) << Lines(DiffFields(a, b));
    EXPECT_EQ(a.Offset, b.Offset);
    EXPECT_EQ(a.Cache, b.Cache);
    EXPECT_EQ((std::vector<int>{1}), b.List);
    EXPECT_EQ(1, b.Map[1]);
    EXPECT_EQ(kPaint_Green, b.Colour);
}

TEST(FieldTable, DerivedFieldsAreComparedOnRequest)
{
    Sample a;
    Sample b;
    b.Offset = Point(4, 5);
    EXPECT_EQ("Offset: (0, 0) != (4, 5)\n", Lines(DiffFields(a, b)));
    EXPECT_EQ("", Lines(DiffFields(a, b, false)));
}

TEST(FieldTable, OpaqueFieldsAreNotCompared)
{
    Sample a;
    Sample b;
    b.Cache = "elsewhere";
    EXPECT_EQ("", Lines(DiffFields(a, b)));
}

TEST(FieldTable, PrintsContainersInOneOrder)
{
    Sample a;
    a.Map = {{9, 1}, {2, 5}, {4, 7}};
    a.List = {3, 1};
    EXPECT_STREQ("{Kind: {Flow: 0}, Number: 0, Small: -1, Letter: 0, Flag: 0, Colour: 0, Name: \"\", "
                 "Anchor: (0.5, 1), Offset: (0, 0), List: [3, 1], Map: {2: 5, 4: 7, 9: 1}, Fixed: [0, 0, 0], "
                 "Corners: [(0, 0), (0, 0)], Nested: {Count: 1, Ratio: 2}}",
                 PrintFields(a).GetCStr());
}

// A rectangle is one value; a map with a hash and a comparison of its own compares its keys with them
TEST(FieldTable, RectanglesAndMapsWithoutCase)
{
    Rect a = RectWH(10, 20, 30, 40);
    Rect b = a;
    FieldValue<Rect>::Alter(b);
    EXPECT_FALSE(FieldValue<Rect>::Equal(a, b));
    EXPECT_EQ(RectWH(11, 21, 31, 41), b);
    EXPECT_STREQ("(11, 21)-(41, 61)", FieldValue<Rect>::Print(b).GetCStr());

    StringIMap tags = {{"Weight", "1"}, {"Colour", "red"}};
    StringIMap same_tags = {{"WEIGHT", "1"}, {"colour", "red"}};
    EXPECT_TRUE(FieldValue<StringIMap>::Equal(tags, same_tags));
    same_tags["colour"] = "Red";
    EXPECT_FALSE(FieldValue<StringIMap>::Equal(tags, same_tags));
    EXPECT_STREQ("{\"Colour\": \"red\", \"Weight\": \"1\"}", FieldValue<StringIMap>::Print(tags).GetCStr());
    FieldValue<StringIMap>::Alter(tags);
    EXPECT_EQ(3u, tags.size());
}

// A std::array, a set and a point in time are values: compared, printed (a set in the order of its keys) and altered;
// a number at the largest value of its type is altered without an overflow
TEST(FieldTable, ArraysSetsTimePointsAndLargestNumbers)
{
    Session a;
    a.Tokens = {"b", "a"};
    a.Until += std::chrono::steady_clock::duration(5);
    EXPECT_STREQ("{Switches: [0, 1], Tokens: [\"a\", \"b\"], Until: 5, ZOrder: 2147483647, SkippedBy: 0}",
                 PrintFields(a).GetCStr());

    Session b = a;
    EXPECT_EQ("", Lines(DiffFields(a, b)));
    AlterFields(b);
    EXPECT_EQ("Switches: [0, 1] != [1, 0]\n"
              "Tokens: [\"a\", \"b\"] != [\"a\", \"b\", \"~\"]\n"
              "Until: 5 != 6\n"
              "ZOrder: 2147483647 != 2147483646\n",
              Lines(DiffFields(a, b)));

    // the key a set gets is one it does not have yet
    std::unordered_set<String> tokens = {"~"};
    FieldValue<std::unordered_set<String>>::Alter(tokens);
    EXPECT_EQ((std::unordered_set<String>{"~", "~~"}), tokens);
}

// A transient field is compared and printed as a saved one, also without the derived fields; AlterFields changes it only
// when asked for that role
TEST(FieldTable, TransientFieldsAreComparedAndAlteredOnRequest)
{
    Session a;
    Session b;
    b.SkippedBy = 2;
    EXPECT_EQ("SkippedBy: 0 != 2\n", Lines(DiffFields(a, b, false)));

    Session saved;
    AlterFields(saved);
    EXPECT_EQ(a.SkippedBy, saved.SkippedBy);

    Session transient;
    AlterFields(transient, FieldRole::Transient);
    EXPECT_EQ("SkippedBy: 0 != 1\n", Lines(DiffFields(a, transient)));
}

// A list of structs with a table (a vector, a plain array, a std::array, a list in an element of a list) is compared
// element by element: a line per field, named with the element's index
TEST(FieldTable, DiffNamesTheElementsOfAListOfStructs)
{
    const Reel a = MakeReel();
    Reel b = a;
    EXPECT_EQ("", Lines(DiffFields(a, b)));
    b.Strips[1].Cels[0].Delay = 4;
    b.Strips[0].CelCount = 3;
    b.Pair[1].Delay = 5;
    b.Ends[0].Reserved = 6;
    b.Template.Ratio = 0.5f;
    b.Skies[1].Width = 7;
    EXPECT_EQ("Strips[0].CelCount: 2 != 3\n"
              "Strips[1].Cels[0].Delay: 0 != 4\n"
              "Pair[1].Delay: 0 != 5\n"
              "Ends[0].Reserved: 0 != 6\n"
              "Template.Ratio: 2 != 0.5\n"
              "Skies[1].Width: 0 != 7\n",
              Lines(DiffFields(a, b)));
    // a derived field of an element is left out with the struct's own
    EXPECT_EQ(5u, DiffFields(a, b, false).size()) << Lines(DiffFields(a, b, false));
}

// Two lists of different lengths differ in one line, without their elements' lines; a list of plain values is one value
TEST(FieldTable, DiffOfListsOfDifferentLengthsIsOneLine)
{
    const Reel a = MakeReel();
    Reel b = a;
    b.Strips[0].Cels.resize(3);
    b.Strips[0].Cels[0].Delay = 1;
    b.Strips[1].Cels[1].Delay = 2;
    b.Numbers = {4, 5};
    EXPECT_EQ("Strips[0].Cels: 2 items != 3 items\n"
              "Strips[1].Cels[1].Delay: 0 != 2\n"
              "Numbers: [] != [4, 5]\n",
              Lines(DiffFields(a, b)));

    Reel c = a;
    c.Strips.resize(3);
    c.Strips[0].CelCount = 4;
    EXPECT_EQ("Strips: 2 items != 3 items\n", Lines(DiffFields(a, c)));
}

// AlterFields descends into every element of a list of structs with the same role and never resizes the list; a list
// of plain values gets an element more as before. A member and a list whose rows say FromDefinition are untouched with
// Saved, though their own rows are Saved.
TEST(FieldTable, AlterReachesEveryElementAndKeepsTheLength)
{
    const Reel a = MakeReel();
    Reel saved = a;
    AlterFields(saved);
    EXPECT_EQ("Strips[0].Cels[0].Delay: 0 != 1\n"
              "Strips[0].Cels[1].Delay: 0 != 1\n"
              "Strips[1].Cels[0].Delay: 0 != 1\n"
              "Strips[1].Cels[1].Delay: 0 != 1\n"
              "Pair[0].Delay: 0 != 1\n"
              "Pair[1].Delay: 0 != 1\n"
              "Ends[0].Delay: 0 != 1\n"
              "Ends[1].Delay: 0 != 1\n"
              "Numbers: [] != [1]\n",
              Lines(DiffFields(a, saved)));
    EXPECT_EQ(2u, saved.Strips.size());
    EXPECT_EQ(2u, saved.Strips[1].Cels.size());
    EXPECT_EQ(2u, saved.Skies.size());

    Reel from_definition = a;
    AlterFields(from_definition, FieldRole::FromDefinition);
    EXPECT_EQ(2u, from_definition.Strips.size());
    EXPECT_EQ(2u, from_definition.Strips[0].Cels.size());
    EXPECT_EQ(2u, from_definition.Skies.size());
}

// A role other than Saved on a member with rows of its own covers the whole member: with FromDefinition every field of
// Template and of each element of Skies is altered, the saved, the derived and the ones from the definition alike
TEST(FieldTable, ARoleCoversTheWholeMember)
{
    const Reel a = MakeReel();
    Reel from_definition = a;
    AlterFields(from_definition, FieldRole::FromDefinition);
    EXPECT_EQ("Strips[0].CelCount: 2 != 3\n"
              "Strips[0].Cels[0].Reserved: 0 != 1\n"
              "Strips[0].Cels[1].Reserved: 0 != 1\n"
              "Strips[1].CelCount: 2 != 3\n"
              "Strips[1].Cels[0].Reserved: 0 != 1\n"
              "Strips[1].Cels[1].Reserved: 0 != 1\n"
              "Pair[0].Reserved: 0 != 1\n"
              "Pair[1].Reserved: 0 != 1\n"
              "Ends[0].Reserved: 0 != 1\n"
              "Ends[1].Reserved: 0 != 1\n"
              "Template.Count: 1 != 2\n"
              "Template.Ratio: 2 != 3\n"
              "Skies[0].Tint: 0 != 1\n"
              "Skies[0].Width: 0 != 1\n"
              "Skies[0].Room: 0 != 1\n"
              "Skies[1].Tint: 0 != 1\n"
              "Skies[1].Width: 0 != 1\n"
              "Skies[1].Room: 0 != 1\n",
              Lines(DiffFields(a, from_definition)));

    // Saved leaves them as they are (AlterReachesEveryElementAndKeepsTheLength); Transient, no row's, alters nothing
    Reel transient = a;
    AlterFields(transient, FieldRole::Transient);
    EXPECT_EQ("", Lines(DiffFields(a, transient)));
}

// A struct with a table that is a map's value has no role but the map's row's: its saved fields are altered
TEST(FieldTable, AMapsValueWithATableAltersItsSavedFields)
{
    std::unordered_map<int, Cel> cels;
    FieldValue<std::unordered_map<int, Cel>>::Alter(cels);
    ASSERT_EQ(1u, cels.size());
    EXPECT_EQ(1, cels.begin()->second.Delay);
    EXPECT_EQ(0, cels.begin()->second.Reserved);
}

// The character's definition: each row names its own member, and the operations reach every one but the two
// event tables
TEST(FieldTable, CharacterDefHasARowPerField)
{
    CharacterDef a;
    CharacterDef b;
    EXPECT_TRUE(FieldsAreDistinct(a));
    EXPECT_EQ("", Lines(DiffFields(a, b)));
    AlterFields(b);
    EXPECT_EQ(CharacterDef::Fields().Count - 2, DiffFields(a, b).size()) << Lines(DiffFields(a, b));
    EXPECT_EQ(1, b.Baseline - a.Baseline);
    EXPECT_EQ(1, b.get_item_quantity(1));
}

// The plain entities' definitions (PLAN.md 5.1a step 3): the rows by role are those of the save code of each entity's
// live copy (Engine/game/save/savegame_components.cpp); the engine's tests GameEntities.Every...FieldIsRestored check
// them against it
TEST(FieldTable, InventoryItemDefHasARowPerField)
{
    ExpectWholeTable<InventoryItemDef>(6, 1, 2);
}

TEST(FieldTable, MouseCursorDefHasARowPerField)
{
    ExpectWholeTable<MouseCursorDef>(6, 2, 0);
}

TEST(FieldTable, ViewDefsHaveARowPerField)
{
    ExpectWholeTable<ViewFrameDef>(7, 1, 0);
    ExpectWholeTable<ViewLoopDef>(1, 2, 0);
    ExpectWholeTable<ViewDef>(1, 1, 0);
}

// A view's loops and their frames are compared and altered frame by frame: the saved fields are the frames', the
// counts and the loops' flags are the definition's, and the lists keep their lengths
TEST(FieldTable, ViewLoopsCompareTheirFrames)
{
    ViewDef a;
    a.Initialize(2);
    a.Loops[1].Initialize(2);
    ViewDef b = a;
    EXPECT_EQ("", Lines(DiffFields(a, b)));
    b.Loops[1].Frames[1].ReservedForFuture[1] = 3;
    EXPECT_EQ("Loops[1].Frames[1].ReservedForFuture: [0, 0] != [0, 3]\n", Lines(DiffFields(a, b)));

    ViewDef saved = a;
    AlterFields(saved);
    // 7 saved fields in each of the two frames
    EXPECT_EQ(14u, DiffFields(a, saved).size()) << Lines(DiffFields(a, saved));
    EXPECT_EQ(0u, Lines(DiffFields(a, saved)).find("Loops[1].Frames[0].Sprite: 0 != 1\n"));

    ViewDef from_definition = a;
    AlterFields(from_definition, FieldRole::FromDefinition);
    EXPECT_EQ("LoopCount: 2 != 3\n"
              "Loops[0].FrameCount: 0 != 1\n"
              "Loops[0].Flags: 0 != 1\n"
              "Loops[1].FrameCount: 2 != 3\n"
              "Loops[1].Flags: 0 != 1\n"
              "Loops[1].Frames[0].ReservedForFuture: [0, 0] != [1, 1]\n"
              "Loops[1].Frames[1].ReservedForFuture: [0, 0] != [1, 1]\n",
              Lines(DiffFields(a, from_definition)));
    EXPECT_EQ(2u, from_definition.Loops.size());
    EXPECT_EQ(2u, from_definition.Loops[1].Frames.size());
}

TEST(FieldTable, AudioDefsHaveARowPerField)
{
    ExpectWholeTable<AudioClipDef>(0, 9, 0);
    ExpectWholeTable<AudioClipTypeDef>(2, 2, 0);
}

TEST(FieldTable, DialogDefsHaveARowPerField)
{
    ExpectWholeTable<DialogOptionDef>(2, 2, 0);
    ExpectWholeTable<DialogDef>(1, 4, 0);
}

// A dialog's options are altered option by option, and the list keeps its length
TEST(FieldTable, DialogsAlterTheirOptions)
{
    DialogDef a;
    a.Options.resize(2);
    DialogDef saved = a;
    AlterFields(saved);
    EXPECT_EQ("Options[0].Text: \"\" != \"~\"\n"
              "Options[0].Flags: 0 != 1\n"
              "Options[1].Text: \"\" != \"~\"\n"
              "Options[1].Flags: 0 != 1\n",
              Lines(DiffFields(a, saved)));
    DialogDef from_definition = a;
    AlterFields(from_definition, FieldRole::FromDefinition);
    EXPECT_EQ(2u, from_definition.Options.size());
    EXPECT_EQ(4u + 2u * 2u, DiffFields(a, from_definition).size()) << Lines(DiffFields(a, from_definition));
}

TEST(FieldTable, FontDefHasARowPerField)
{
    ExpectWholeTable<FontDef>(0, 13, 0);
}
