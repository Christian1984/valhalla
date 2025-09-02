#include "baldr/graphconstants.h"
#include "gurka.h"
#include "test.h"

#include <gtest/gtest.h>
#include <valhalla/midgard/logging.h>

using namespace valhalla;

class UseCurvatureTest : public ::testing::Test {
protected:
  static gurka::map speed_map;

  static void SetUpTestSuite() {
    constexpr double gridsize = 50;

    const std::string ascii_map = R"(
      A---------G
      |        /
      B     E-F
       \   /
        C-D
    )";

    const gurka::ways ways = {{"AG", {{"highway", "primary"}, {"name", "Straight Road"}}},
                              {"ABCDEFG", {{"highway", "primary"}, {"name", "Twisty Road"}}}};

    const auto layout = gurka::detail::map_to_coordinates(ascii_map, gridsize);

    speed_map = gurka::buildtiles(layout, ways, {}, {}, "test/data/usecurvature");
  }

  inline float getDuration(const valhalla::Api& route) {
    return route.directions().routes(0).legs(0).summary().time();
  }
};

gurka::map UseCurvatureTest::speed_map = {};

TEST_F(UseCurvatureTest, CurvatureHighway) {

  std::unordered_map<std::string, std::string> options = 
      {{"/costing_options/motorcycle/use_curvature", "100"}};

  logging::LoggingConfig logconf = {
    {"type", "file"},                            // write to a file (bypasses stdout/stderr capture)
    {"file_name", "valhalla.route.log"},
    {"color", "false"}
  };
  logging::Configure(logconf);
  LOG_INFO("logger configured (smoke)"); 

  valhalla::Api default_route =
      gurka::do_action(valhalla::Options::route, speed_map, {"A", "G"}, "motorcycle");

  valhalla::Api curvy_route =
      gurka::do_action(valhalla::Options::route, speed_map, {"A", "G"}, "motorcycle", options);

  gurka::assert::raw::expect_path(default_route, {"Straight Road"});
  gurka::assert::raw::expect_path(curvy_route, {"Twisty Road"});
}
