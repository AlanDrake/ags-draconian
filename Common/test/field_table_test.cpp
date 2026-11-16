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
