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
// The engine's route finder (MaskRouteFinder, the jump point search one) on
// small walkable masks drawn as text: straight lines, paths around walls,
// unreachable and blocked destinations, coordinate scaling, and the search
// for the nearest walkable point. Paths are checked for what the engine needs
// from them (the right ends, every leg a walkable straight line) and against
// the shortest grid path, rather than for exact waypoints.
//
//=============================================================================
#include <cmath>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "ac/route_finder.h"
#include "gfx/bitmap.h"
#include "gtest/gtest.h"

using namespace AGS::Common;
using namespace AGS::Engine;

namespace
{

// An 8-bit walkable mask from rows of text: '#' is a wall (0), any other character is area 1
std::unique_ptr<Bitmap> MakeMask(const std::vector<std::string> &rows)
{
    const int height = static_cast<int>(rows.size());
    const int width = static_cast<int>(rows[0].size());
    auto mask = std::make_unique<Bitmap>(width, height, 8);
    mask->Clear(0);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            if (rows[y][x] != '#')
                mask->PutPixel(x, y, 1);
        }
    }
    return mask;
}

// A mask of the given size with no walls
std::unique_ptr<Bitmap> MakeOpenMask(int width, int height)
{
    auto mask = std::make_unique<Bitmap>(width, height, 8);
    mask->Clear(1);
    return mask;
}

std::unique_ptr<MaskRouteFinder> MakeFinder(const Bitmap *mask, int coord_scale = 1)
{
    auto finder = Pathfinding::CreateDefaultMaskPathfinder();
    finder->SetWalkableArea(mask, coord_scale);
    return finder;
}

double PathLength(const std::vector<Point> &path)
{
    double length = 0.0;
    for (size_t i = 1; i < path.size(); ++i)
        length += std::hypot(path[i].X - path[i - 1].X, path[i].Y - path[i - 1].Y);
    return length;
}

// Shortest path on the mask's grid, moving to the 8 neighbours with cost 1 or sqrt(2);
// no diagonal step past a wall corner, as the engine's characters cannot do it either.
// Returns a negative value if the destination cannot be reached.
double ShortestGridPath(const Bitmap *mask, const Point &src, const Point &dst)
{
    const int w = mask->GetWidth();
    const int h = mask->GetHeight();
    auto walkable = [&](int x, int y) { return x >= 0 && y >= 0 && x < w && y < h && mask->GetPixel(x, y) != 0; };
    std::vector<double> dist(w * h, -1.0);
    using Entry = std::pair<double, int>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;
    dist[src.Y * w + src.X] = 0.0;
    queue.emplace(0.0, src.Y * w + src.X);
    while (!queue.empty())
    {
        const auto [d, idx] = queue.top();
        queue.pop();
        const int x = idx % w, y = idx / w;
        if (d > dist[idx])
            continue;
        if (x == dst.X && y == dst.Y)
            return d;
        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                if ((dx == 0 && dy == 0) || !walkable(x + dx, y + dy))
                    continue;
                if (dx != 0 && dy != 0 && !walkable(x + dx, y) && !walkable(x, y + dy))
                    continue;
                const double nd = d + ((dx != 0 && dy != 0) ? std::sqrt(2.0) : 1.0);
                const int nidx = (y + dy) * w + (x + dx);
                if (dist[nidx] < 0.0 || nd < dist[nidx])
                {
                    dist[nidx] = nd;
                    queue.emplace(nd, nidx);
                }
            }
        }
    }
    return -1.0;
}

// The path starts and ends where expected, its waypoints are walkable, and each leg is a
// straight walkable line (as the route finder itself traces it)
void ExpectWalkablePath(IRouteFinder &finder, const std::vector<Point> &path, const Point &src, const Point &dst)
{
    ASSERT_GE(path.size(), 2u);
    EXPECT_EQ(src, path.front());
    EXPECT_EQ(dst, path.back());
    for (size_t i = 0; i < path.size(); ++i)
    {
        EXPECT_TRUE(finder.IsWalkableAt(path[i].X, path[i].Y)) << "waypoint " << i;
        if (i > 0 && path[i] != path[i - 1])
        {
            EXPECT_TRUE(finder.CanSeeFrom(path[i - 1].X, path[i - 1].Y, path[i].X, path[i].Y))
                << "leg " << i - 1 << ": " << path[i - 1].X << "," << path[i - 1].Y << " -> " << path[i].X << ","
                << path[i].Y;
        }
    }
}

// A room split by a wall with a gap at the bottom
const std::vector<std::string> kWallWithGap = {
    "....................", //
    ".........#..........", //
    ".........#..........", //
    ".........#..........", //
    ".........#..........", //
    ".........#..........", //
    ".........#..........", //
    ".........#..........", //
    ".........#..........", //
    "....................", //
};

} // namespace

TEST(RouteFinder, StraightLineOnOpenMask)
{
    const auto mask = MakeOpenMask(40, 30);
    const auto finder = MakeFinder(mask.get());
    std::vector<Point> path;
    ASSERT_TRUE(finder->FindRoute(path, 2, 3, 35, 27));
    ASSERT_EQ(2u, path.size());
    EXPECT_EQ(Point(2, 3), path[0]);
    EXPECT_EQ(Point(35, 27), path[1]);
    EXPECT_TRUE(finder->CanSeeFrom(2, 3, 35, 27));
}

TEST(RouteFinder, RouteToTheStartPointIsTwoEqualPoints)
{
    const auto mask = MakeOpenMask(10, 10);
    const auto finder = MakeFinder(mask.get());
    std::vector<Point> path;
    ASSERT_TRUE(finder->FindRoute(path, 4, 4, 4, 4));
    ASSERT_EQ(2u, path.size());
    EXPECT_EQ(Point(4, 4), path[0]);
    EXPECT_EQ(Point(4, 4), path[1]);
}

TEST(RouteFinder, PathGoesAroundAWall)
{
    const auto mask = MakeMask({
        "....................", //
        ".........#..........", //
        ".........#..........", //
        ".........#..........", //
        ".........#..........", //
        ".........#..........", //
        ".........#..........", //
        ".........#..........", //
        ".........#..........", //
        "....................", //
    });
    const auto finder = MakeFinder(mask.get());
    EXPECT_FALSE(finder->CanSeeFrom(2, 5, 17, 5));
    std::vector<Point> path;
    ASSERT_TRUE(finder->FindRoute(path, 2, 5, 17, 5));
    EXPECT_GT(path.size(), 2u);
    ExpectWalkablePath(*finder, path, Point(2, 5), Point(17, 5));
    const double shortest = ShortestGridPath(mask.get(), Point(2, 5), Point(17, 5));
    ASSERT_GT(shortest, 0.0);
    EXPECT_LE(PathLength(path), shortest + 0.01);
}

TEST(RouteFinder, PathFollowsAWindingCorridor)
{
    // jump point search has to turn at every corner of the corridor
    const auto mask = MakeMask({
        "...#.......#.......#", //
        "...#.......#.......#", //
        "...#...#...#...#...#", //
        "...#...#...#...#...#", //
        "...#...#...#...#...#", //
        "...#...#...#...#...#", //
        "...#...#...#...#...#", //
        "...#...#...#...#...#", //
        ".......#.......#....", //
        ".......#.......#....", //
    });
    const auto finder = MakeFinder(mask.get());
    const Point src(1, 1), dst(18, 9);
    std::vector<Point> path;
    ASSERT_TRUE(finder->FindRoute(path, src.X, src.Y, dst.X, dst.Y));
    ExpectWalkablePath(*finder, path, src, dst);
    const double shortest = ShortestGridPath(mask.get(), src, dst);
    ASSERT_GT(shortest, 0.0);
    EXPECT_LE(PathLength(path), shortest + 0.01);
    // the corridor is 8 rows high and turns 4 times: no path can be shorter than its legs
    EXPECT_GE(PathLength(path), 4 * 6.0);
}

TEST(RouteFinder, PathsAreShortOnARandomMask)
{
    // a fixed pseudo-random scatter of obstacles; compare with the shortest grid path
    std::vector<std::string> rows(30, std::string(40, '.'));
    uint32_t seed = 12345u;
    for (int i = 0; i < 300; ++i)
    {
        seed = seed * 1103515245u + 12345u;
        const int x = (seed >> 8) % 40;
        seed = seed * 1103515245u + 12345u;
        const int y = (seed >> 8) % 30;
        rows[y][x] = '#';
    }
    rows[0][0] = '.';
    rows[29][39] = '.';
    const auto mask = MakeMask(rows);
    const auto finder = MakeFinder(mask.get());
    const Point pairs[][2] = {{{0, 0}, {39, 29}}, {{39, 29}, {0, 0}}, {{0, 29}, {39, 0}}, {{20, 0}, {20, 29}}};
    for (const auto &pair : pairs)
    {
        const Point src = pair[0], dst = pair[1];
        if (mask->GetPixel(src.X, src.Y) == 0 || mask->GetPixel(dst.X, dst.Y) == 0)
            continue;
        const double shortest = ShortestGridPath(mask.get(), src, dst);
        if (shortest < 0.0)
            continue; // not connected in this scatter
        std::vector<Point> path;
        ASSERT_TRUE(finder->FindRoute(path, src.X, src.Y, dst.X, dst.Y));
        ExpectWalkablePath(*finder, path, src, dst);
        EXPECT_LE(PathLength(path), shortest + 0.01) << src.X << "," << src.Y << " -> " << dst.X << "," << dst.Y;
    }
}

TEST(RouteFinder, NoDiagonalStepBetweenWallCorners)
{
    // the two areas touch only at a corner: (3,2) and (4,3)
    const auto mask = MakeMask({
        "....#####", //
        "....#####", //
        "....#####", //
        "####.....", //
        "####.....", //
    });
    const auto finder = MakeFinder(mask.get());
    EXPECT_FALSE(finder->CanSeeFrom(3, 2, 4, 3));
    EXPECT_LT(ShortestGridPath(mask.get(), Point(1, 1), Point(7, 4)), 0.0);
    std::vector<Point> path;
    if (finder->FindRoute(path, 1, 1, 7, 4))
    {
        // the route stops on the start's side
        ASSERT_FALSE(path.empty());
        EXPECT_LE(path.back().X, 3);
        EXPECT_LE(path.back().Y, 2);
    }
}

TEST(RouteFinder, UnreachableDestinationGivesRouteToTheClosestPoint)
{
    // the wall splits the room in two
    const auto mask = MakeMask({
        ".........#..........", //
        ".........#..........", //
        ".........#..........", //
        ".........#..........", //
        ".........#..........", //
    });
    const auto finder = MakeFinder(mask.get());
    std::vector<Point> path;
    ASSERT_TRUE(finder->FindRoute(path, 2, 2, 15, 2));
    ASSERT_FALSE(path.empty());
    EXPECT_EQ(Point(2, 2), path.front());
    EXPECT_EQ(Point(8, 2), path.back());
}

TEST(RouteFinder, DestinationInAWall)
{
    const auto mask = MakeMask(kWallWithGap);
    const auto finder = MakeFinder(mask.get());
    std::vector<Point> path;
    // exact destination: refused
    EXPECT_FALSE(finder->FindRoute(path, 2, 5, 9, 5, true));
    // otherwise the route goes as close as it can
    ASSERT_TRUE(finder->FindRoute(path, 2, 5, 9, 5, false));
    ASSERT_FALSE(path.empty());
    EXPECT_EQ(Point(8, 5), path.back());
}

TEST(RouteFinder, StartInAWallHasNoRoute)
{
    const auto mask = MakeMask(kWallWithGap);
    const auto finder = MakeFinder(mask.get());
    std::vector<Point> path;
    EXPECT_FALSE(finder->FindRoute(path, 9, 5, 15, 5));
    EXPECT_FALSE(finder->FindRoute(path, -5, 5, 15, 5));
}

TEST(RouteFinder, IgnoringWallsGivesAStraightLine)
{
    const auto mask = MakeMask(kWallWithGap);
    const auto finder = MakeFinder(mask.get());
    std::vector<Point> path;
    ASSERT_TRUE(finder->FindRoute(path, 2, 5, 17, 5, false, true));
    ASSERT_EQ(2u, path.size());
    EXPECT_EQ(Point(2, 5), path[0]);
    EXPECT_EQ(Point(17, 5), path[1]);
}

TEST(RouteFinder, CanSeeFromStopsAtTheWall)
{
    const auto mask = MakeMask(kWallWithGap);
    const auto finder = MakeFinder(mask.get());
    int last_x = -1, last_y = -1;
    EXPECT_FALSE(finder->CanSeeFrom(2, 5, 17, 5, &last_x, &last_y));
    EXPECT_EQ(8, last_x);
    EXPECT_EQ(5, last_y);
    // the bottom row is open
    EXPECT_TRUE(finder->CanSeeFrom(2, 9, 17, 9, &last_x, &last_y));
    // a point does not "see" itself
    EXPECT_FALSE(finder->CanSeeFrom(2, 9, 2, 9));
}

TEST(RouteFinder, ScaledMaskWorksInRoomCoordinates)
{
    // a mask at half the room resolution: route coordinates are room coordinates
    const auto mask = MakeMask(kWallWithGap);
    const auto finder = MakeFinder(mask.get(), 2);
    EXPECT_TRUE(finder->IsWalkableAt(4, 10));
    EXPECT_FALSE(finder->IsWalkableAt(18, 10));
    EXPECT_FALSE(finder->IsWalkableAt(19, 11));
    std::vector<Point> path;
    ASSERT_TRUE(finder->FindRoute(path, 4, 10, 34, 10));
    ASSERT_GT(path.size(), 2u);
    for (const auto &pt : path)
    {
        EXPECT_EQ(0, pt.X % 2);
        EXPECT_EQ(0, pt.Y % 2);
        EXPECT_TRUE(finder->IsWalkableAt(pt.X, pt.Y));
    }
    EXPECT_EQ(Point(4, 10), path.front());
    EXPECT_EQ(Point(34, 10), path.back());
    // odd room coordinates are rounded down to the mask's pixel
    ASSERT_TRUE(finder->FindRoute(path, 5, 11, 35, 11));
    EXPECT_EQ(Point(4, 10), path.front());
    EXPECT_EQ(Point(34, 10), path.back());
}

TEST(RouteFinder, NoMaskNoRoute)
{
    auto finder = Pathfinding::CreateDefaultMaskPathfinder();
    std::vector<Point> path;
    EXPECT_FALSE(finder->FindRoute(path, 0, 0, 1, 1));
    EXPECT_FALSE(finder->CanSeeFrom(0, 0, 1, 1));
    EXPECT_FALSE(finder->IsWalkableAt(0, 0));
    Point pt;
    EXPECT_FALSE(finder->FindNearestWalkablePoint(Point(0, 0), pt));
}

TEST(NearestWalkablePoint, StartingPointThatIsWalkable)
{
    const auto mask = MakeMask(kWallWithGap);
    Point found(-1, -1);
    ASSERT_TRUE(Pathfinding::FindNearestWalkablePoint(mask.get(), Point(3, 3), found));
    EXPECT_EQ(Point(3, 3), found);
}

TEST(NearestWalkablePoint, FindsTheClosestPixel)
{
    const auto mask = MakeMask({
        "############", //
        "############", //
        "############", //
        "############", //
        "############", //
        "############", //
        "########.###", //
        "############", //
        "###.########", //
        "############", //
        "############", //
        "############", //
    });
    // both search methods, starting where the scanned squares stay inside the mask
    for (bool method362 : {true, false})
    {
        Point found(-1, -1);
        ASSERT_TRUE(Pathfinding::FindNearestWalkablePoint(mask.get(), Point(7, 4), found, 0, 1, method362));
        EXPECT_EQ(Point(8, 6), found) << "method362 " << method362;
        ASSERT_TRUE(Pathfinding::FindNearestWalkablePoint(mask.get(), Point(4, 6), found, 0, 1, method362));
        EXPECT_EQ(Point(3, 8), found) << "method362 " << method362;
        ASSERT_TRUE(Pathfinding::FindNearestWalkablePoint(mask.get(), Point(5, 9), found, 0, 1, method362));
        EXPECT_EQ(Point(3, 8), found) << "method362 " << method362;
    }
}

// FIXME: disabled, it fails. The 3.6.2 search (use_method362) scans squares of growing size around the
// start; inside the square's columns it visits only the first scanned row and the row 2 * range
// below it. When the square is cut by the top of the mask, the first scanned row is the mask's
// top instead of the square's top, so the square's bottom row is never visited: here (6,3) is
// missed and the farther (1,5) is returned. Characters placed near the top of a room snap to
// a farther point than the nearest. The pre-3.6.2 search finds (6,3).
TEST(NearestWalkablePoint, DISABLED_FindsTheClosestPixelNearTheTopEdge)
{
    const auto mask = MakeMask({
        "##########", //
        "##########", //
        "##########", //
        "######.###", //
        "##########", //
        "#.########", //
    });
    Point found(-1, -1);
    ASSERT_TRUE(Pathfinding::FindNearestWalkablePoint(mask.get(), Point(5, 1), found, 0, 1, false));
    EXPECT_EQ(Point(6, 3), found);
    ASSERT_TRUE(Pathfinding::FindNearestWalkablePoint(mask.get(), Point(5, 1), found, 0, 1, true));
    EXPECT_EQ(Point(6, 3), found);
}

TEST(NearestWalkablePoint, RangeAndLimitsRestrictTheSearch)
{
    const auto mask = MakeMask({
        "##########", //
        "##########", //
        "##########", //
        "##########", //
        "##########", //
        "#########.", //
    });
    Point found(-1, -1);
    EXPECT_TRUE(Pathfinding::FindNearestWalkablePoint(mask.get(), Point(1, 1), found));
    EXPECT_EQ(Point(9, 5), found);
    // a range of 4 around (1,1) does not reach (9,5)
    EXPECT_FALSE(Pathfinding::FindNearestWalkablePoint(mask.get(), Point(1, 1), found, 4, 1));
    // limits that leave the walkable pixel out
    EXPECT_FALSE(Pathfinding::FindNearestWalkablePoint(mask.get(), Point(1, 1), found, RectWH(0, 0, 8, 6), 0, 1));
    EXPECT_TRUE(Pathfinding::FindNearestWalkablePoint(mask.get(), Point(1, 1), found, RectWH(5, 3, 5, 3), 0, 1));
    EXPECT_EQ(Point(9, 5), found);
}

TEST(NearestWalkablePoint, NothingWalkable)
{
    const auto mask = MakeMask({"#####", "#####"});
    Point found(-1, -1);
    EXPECT_FALSE(Pathfinding::FindNearestWalkablePoint(mask.get(), Point(2, 1), found));
    EXPECT_EQ(Point(-1, -1), found);
}

TEST(NearestWalkablePoint, ThroughTheRouteFinderWithScaling)
{
    const auto mask = MakeMask({
        "#####", //
        "#####", //
        "#####", //
        "#####", //
        "###..", //
    });
    const auto finder = MakeFinder(mask.get(), 3);
    Point found(-1, -1);
    // room (10,7) is mask (3,2); the nearest walkable mask pixel (3,4) is room (9,12)
    ASSERT_TRUE(finder->FindNearestWalkablePoint(Point(10, 7), found));
    EXPECT_EQ(Point(9, 12), found);
}
