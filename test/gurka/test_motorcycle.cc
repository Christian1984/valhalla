#include "gurka.h"
#include "test.h"

#include <gtest/gtest.h>

using namespace valhalla;

// Tests for motorcycle-specific costing options that affect road type selection.
// These options are particularly useful for "Landstraße" routing: preferring rural,
// winding country roads over urban neighborhood streets when use_curvature is high.

class MotorcycleResidentialTest : public ::testing::Test {
protected:
  static gurka::map map;

  static void SetUpTestSuite() {
    // Map with two routes from 1 to 2:
    //   Short path:   1 -> AB -> BE -> EF -> 2  (BE is residential, direct)
    //   Long bypass:  1 -> AB -> BC -> CD -> DE -> EF -> 2  (all primary, ~2x longer)
    //
    // With residential_factor > 2x (use_residential < ~0.3), the penalty on BE
    // makes the longer primary bypass cheaper. With low penalty (use_residential=1),
    // the shorter residential path wins.
    const std::string ascii_map = R"(
    A-1----B--------E-----2-F
           |        |
           |        |
           |        |
           |        |
           C--------D
    )";

    const gurka::ways ways = {
        {"A1", {{"highway", "primary"}}},
        {"AB", {{"highway", "primary"}}},
        {"BE", {{"highway", "residential"}}},
        {"EF", {{"highway", "primary"}}},
        {"E2", {{"highway", "primary"}}},
        {"BC", {{"highway", "primary"}}},
        {"CD", {{"highway", "primary"}}},
        {"DE", {{"highway", "primary"}}},
    };

    const auto layout = gurka::detail::map_to_coordinates(ascii_map, 100);
    map = gurka::buildtiles(layout, ways, {}, {}, "test/data/gurka_motorcycle");

    // Set uniform speeds so travel time differences come purely from path length and cost factors.
    test::customize_historical_traffic(map.config, [](baldr::DirectedEdge& e) {
      e.set_free_flow_speed(50);
      e.set_constrained_flow_speed(50);
      return std::nullopt;
    });
  }
};

gurka::map MotorcycleResidentialTest::map = {};

TEST_F(MotorcycleResidentialTest, DefaultAvoidsResidential) {
  // Default use_residential=0.1 → factor ~2.6 on residential edge BE (~800m).
  // Cost via BE: ~800 * 2.6 = 2080 > primary bypass ~1600m → takes primary bypass.
  auto result = gurka::do_action(valhalla::Options::route, map, {"1", "2"}, "motorcycle");
  gurka::assert::raw::expect_path(result, {"AB", "BC", "CD", "DE", "E2"});
}

TEST_F(MotorcycleResidentialTest, AvoidResidential) {
  // use_residential=0 → factor = 3.0, strong avoidance of residential road BE.
  auto result = gurka::do_action(valhalla::Options::route, map, {"1", "2"}, "motorcycle",
                                 {{"/costing_options/motorcycle/use_residential", "0"}});
  gurka::assert::raw::expect_path(result, {"AB", "BC", "CD", "DE", "E2"});
}

TEST_F(MotorcycleResidentialTest, PreferResidential) {
  // use_residential=1 → factor = 0.75, residential roads slightly preferred.
  // Cost via BE: ~800 * 0.75 = 600 < primary bypass ~1600m → takes residential shortcut.
  auto result = gurka::do_action(valhalla::Options::route, map, {"1", "2"}, "motorcycle",
                                 {{"/costing_options/motorcycle/use_residential", "1"}});
  gurka::assert::raw::expect_path(result, {"AB", "BE", "E2"});
}

// ---- Living street tests for motorcycle ----

class MotorcycleLivingStreetTest : public ::testing::Test {
protected:
  static gurka::map map;

  static void SetUpTestSuite() {
    // Mirrors the structure of the residential test but using living_street vs residential bypass.
    // Short path via living street BE; long primary bypass via BC->CD->DE.
    const std::string ascii_map = R"(
    A-1----B--------E-----2-F
           |        |
           |        |
           |        |
           |        |
           C--------D
    )";

    const gurka::ways ways = {
        {"A1", {{"highway", "primary"}}},
        {"AB", {{"highway", "primary"}}},
        {"BE", {{"highway", "living_street"}}},
        {"EF", {{"highway", "primary"}}},
        {"E2", {{"highway", "primary"}}},
        {"BC", {{"highway", "primary"}}},
        {"CD", {{"highway", "primary"}}},
        {"DE", {{"highway", "primary"}}},
    };

    const auto layout = gurka::detail::map_to_coordinates(ascii_map, 100);
    map = gurka::buildtiles(layout, ways, {}, {}, "test/data/gurka_motorcycle_living_street");

    test::customize_historical_traffic(map.config, [](baldr::DirectedEdge& e) {
      e.set_free_flow_speed(50);
      e.set_constrained_flow_speed(50);
      return std::nullopt;
    });
  }
};

gurka::map MotorcycleLivingStreetTest::map = {};

TEST_F(MotorcycleLivingStreetTest, DefaultAvoidsLivingStreets) {
  // Default use_living_streets=0.1 → factor > 1.0 on living street BE → takes primary bypass.
  auto result = gurka::do_action(valhalla::Options::route, map, {"1", "2"}, "motorcycle");
  gurka::assert::raw::expect_path(result, {"AB", "BC", "CD", "DE", "E2"});
}

TEST_F(MotorcycleLivingStreetTest, AvoidLivingStreets) {
  auto result = gurka::do_action(valhalla::Options::route, map, {"1", "2"}, "motorcycle",
                                 {{"/costing_options/motorcycle/use_living_streets", "0"}});
  gurka::assert::raw::expect_path(result, {"AB", "BC", "CD", "DE", "E2"});
}

TEST_F(MotorcycleLivingStreetTest, PreferLivingStreets) {
  auto result = gurka::do_action(valhalla::Options::route, map, {"1", "2"}, "motorcycle",
                                 {{"/costing_options/motorcycle/use_living_streets", "1"}});
  gurka::assert::raw::expect_path(result, {"AB", "BE", "E2"});
}
