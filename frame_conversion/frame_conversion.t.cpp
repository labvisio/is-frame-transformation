#include <cmath>
#include <iostream>
#include <random>
#include "frame_conversion.hpp"
#include "google/protobuf/util/message_differencer.h"
#include "gtest/gtest.h"
#include "is/msgs/cv.hpp"

namespace {

auto create_random_tf_matrix() -> cv::Mat {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dist(0.0, 10.0);
  auto rng = [&]() -> float { return dist(gen); };

  // clang-format off
  auto theta = rng();
  return (cv::Mat_<double>(4, 4) 
    << 1,               0,               0,  rng(), 
       0, std::cos(theta), -std::sin(theta), rng(),
       0, std::sin(theta), +std::cos(theta), rng(),
       0,               0,                0,     1
  );
  // clang-format on
}

auto matrices_are_equal(cv::Mat const& l, cv::Mat const& r) -> bool {
  std::cout << l << std::endl;
  std::cout << r << std::endl;
  auto diff = cv::Mat{};
  cv::absdiff(l, r, diff);
  auto less = cv::Mat{diff < 1e4};
  std::cout << less << std::endl;
  return cv::countNonZero(less) == less.rows * less.cols;
}

TEST(FrameConversion, Interface) {
  auto cv1 = create_random_tf_matrix();
  auto cv2 = create_random_tf_matrix();
  auto cv3 = create_random_tf_matrix();

  is::vision::FrameTransformation tf1;
  tf1.set_from(1000);
  tf1.set_to(1);
  *tf1.mutable_tf() = is::to_tensor(cv1);

  is::vision::FrameTransformation tf2;
  tf2.set_from(1001);
  tf2.set_to(1);
  *tf2.mutable_tf() = is::to_tensor(cv2);

  is::vision::FrameTransformation tf3;
  tf3.set_from(1002);
  tf3.set_to(2);
  *tf3.mutable_tf() = is::to_tensor(cv3);

  is::FrameConversion conversions;
  conversions.update_transformation(tf1);
  conversions.update_transformation(tf2);
  conversions.update_transformation(tf3);
  // Graph: 1000 <-> 1 <-> 1001     1002 <-> 2

  // Find path between the transformations we just added
  auto path = conversions.find_path(is::Edge{1001, 1000});
  ASSERT_TRUE(path);

  // The path we expected to find
  auto expected_path = is::Path{1001, 1, 1000};
  ASSERT_EQ(*path, expected_path);

  // Transformation computed by the algorithm
  auto composed_tf = is::to_mat(conversions.compose_path(*path));
  // Transformation we expect to be returned
  auto expected_tf = cv1.inv() * cv2;
  ASSERT_TRUE(matrices_are_equal(composed_tf, expected_tf));

  path = conversions.find_path(is::Path{1001, 1000});
  ASSERT_TRUE(path);
  ASSERT_EQ(*path, expected_path);

  // Request paths that does not exist
  // -> Using edges
  path = conversions.find_path(is::Edge{3000, 1000});
  ASSERT_FALSE(path);
  ASSERT_EQ(path.error(), "Invalid frame \"3000\"");

  path = conversions.find_path(is::Edge{1000, 3000});
  ASSERT_FALSE(path);
  ASSERT_EQ(path.error(), "Invalid frame \"3000\"");

  path = conversions.find_path(is::Edge{1000, 1002});
  ASSERT_FALSE(path);
  ASSERT_EQ(path.error(), "Frames \"1000\" and \"1002\" are not connected");

  // -> Using paths
  path = conversions.find_path(is::Path{3000, 1000});
  ASSERT_FALSE(path);
  ASSERT_EQ(path.error(), "Invalid frame \"3000\"");

  path = conversions.find_path(is::Path{1000, 3000});
  ASSERT_FALSE(path);
  ASSERT_EQ(path.error(), "Invalid frame \"3000\"");

  path = conversions.find_path(is::Path{1000, 1002});
  ASSERT_FALSE(path);
  ASSERT_EQ(path.error(), "Frames \"1000\" and \"1002\" are not connected");

  // Test find_path with paths larger than 2 nodes
  auto cv4 = create_random_tf_matrix();
  auto cv5 = create_random_tf_matrix();
  auto cv6 = create_random_tf_matrix();
  auto cv7 = create_random_tf_matrix();

  is::vision::FrameTransformation tf4;
  tf4.set_from(1000);
  tf4.set_to(2);
  *tf4.mutable_tf() = is::to_tensor(cv4);

  is::vision::FrameTransformation tf5;
  tf5.set_from(1001);
  tf5.set_to(2);
  *tf5.mutable_tf() = is::to_tensor(cv5);

  is::vision::FrameTransformation tf6;
  tf6.set_from(1001);
  tf6.set_to(1004);
  *tf6.mutable_tf() = is::to_tensor(cv6);

  conversions.update_transformation(tf4);
  conversions.update_transformation(tf5);
  conversions.update_transformation(tf6);

  // Test transformation update, result should use new matrix
  *tf6.mutable_tf() = is::to_tensor(cv7);
  conversions.update_transformation(tf6);

  /* Graph:
                    1004
                      |
      1000 <-> 1 <-> 1001 <-> 2 <-> 1002
        |_____________________|
  */

  path = conversions.find_path(is::Path{1000, 2, 1004});
  ASSERT_TRUE(path);

  expected_path = is::Path{1000, 2, 1001, 1004};
  ASSERT_EQ(*path, expected_path);

  // Transformation computed by the algorithm
  composed_tf = is::to_mat(conversions.compose_path(*path));
  // Transformation we expect to be returned
  expected_tf = cv7 * cv5.inv() * cv4;
  ASSERT_TRUE(matrices_are_equal(composed_tf, expected_tf));

  // Remove transformations
  conversions.remove_transformation(tf6);  // Trying to remove twice should have no effect
  conversions.remove_transformation(tf6);
  path = conversions.find_path(is::Path{1000, 2, 1004});
  ASSERT_FALSE(path);
  ASSERT_EQ(path.error(), "Frames \"2\" and \"1004\" are not connected");

  // Add the path again
  conversions.update_transformation(tf6);
  path = conversions.find_path(is::Path{1000, 2, 1004});
  ASSERT_TRUE(path);
}

}  // namespace