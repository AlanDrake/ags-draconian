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
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "game/character_def.h"
#include "gtest/gtest.h"
#include "util/field_table.h"
#include "util/geometry.h"
#include "util/string.h"

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

std::string Lines(const std::vector<String> &lines)
{
    std::string text;
    for (const String &line : lines)
        text += std::string(line.GetCStr()) + "\n";
    return text;
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
    EXPECT_EQ("Kind: {Flow: 0} != {Flow: 3}\n"
              "Small: -1 != 7\n"
              "Name: \"\" != \"Roger\"\n"
              "Nested: {Count: 1, Ratio: 2} != {Count: 1, Ratio: 0.5}\n",
              Lines(DiffFields(a, b)));
}

TEST(FieldTable, AlterChangesEverySavedFieldAndNoOther)
{
    Sample a;
    Sample b;
    AlterFields(b);
    // 15 rows, less the derived one and the opaque one
    EXPECT_EQ(13u, DiffFields(a, b).size()) << Lines(DiffFields(a, b));
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
