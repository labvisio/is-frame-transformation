#include <iostream>
#include "frame_conversion.hpp"
#include "google/protobuf/util/message_differencer.h"
#include "gtest/gtest.h"
#include "is/msgs/cv.hpp"

namespace {

TEST(FrameConversion, Interface) {
  is::vision::FrameTransformation tf1;
  tf1.set_from(1000);
  tf1.set_to(1);
  auto cv1 = 2 * cv::Mat::eye(4, 4, CV_64F);
  *tf1.mutable_tf() = is::to_tensor(cv1);

  is::vision::FrameTransformation tf2;
  tf2.set_from(1001);
  tf2.set_to(1);
  auto cv2 = 3 * cv::Mat::eye(4, 4, CV_64F);
  *tf2.mutable_tf() = is::to_tensor(cv2);

  is::vision::FrameTransformation tf3;
  tf3.set_from(1002);
  tf3.set_to(2);
  auto cv3 = 4 * cv::Mat::eye(4, 4, CV_64F);
  *tf3.mutable_tf() = is::to_tensor(cv3);

  is::FrameConversion conversions;
  conversions.update_transformation(tf1);
  conversions.update_transformation(tf2);
  conversions.update_transformation(tf3);

  auto path = conversions.find_path(is::Edge{1001, 1000});
  ASSERT_TRUE(path);
  auto expected_path = is::Path{1001, 1, 1000};
  ASSERT_EQ(*path, expected_path);
  ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(conversions.compose_path(*path),
                                                                 is::to_tensor(cv1.inv() * cv2)));

  path = conversions.find_path(is::Edge{3000, 1000});
  ASSERT_FALSE(path);
  ASSERT_EQ(path.error(), "Invalid frame \"3000\"");

  path = conversions.find_path(is::Edge{1000, 3000});
  ASSERT_FALSE(path);
  ASSERT_EQ(path.error(), "Invalid frame \"3000\"");

  path = conversions.find_path(is::Edge{1000, 1002});
  ASSERT_FALSE(path);
  ASSERT_EQ(path.error(), "Frames \"1000\" and \"1002\" are not connected");

  path = conversions.find_path(is::Path{3000, 1000});
  ASSERT_FALSE(path);
  ASSERT_EQ(path.error(), "Invalid frame \"3000\"");

  path = conversions.find_path(is::Path{1000, 3000});
  ASSERT_FALSE(path);
  ASSERT_EQ(path.error(), "Invalid frame \"3000\"");

  path = conversions.find_path(is::Path{1000, 1002});
  ASSERT_FALSE(path);
  ASSERT_EQ(path.error(), "Frames \"1000\" and \"1002\" are not connected");

  path = conversions.find_path(is::Path{1001, 1000});
  ASSERT_TRUE(path);
  ASSERT_EQ(*path, expected_path);

  is::vision::FrameTransformation tf4;
  tf4.set_from(1000);
  tf4.set_to(2);
  *tf4.mutable_tf() = is::to_tensor(cv1);

  is::vision::FrameTransformation tf5;
  tf5.set_from(1001);
  tf5.set_to(2);
  *tf5.mutable_tf() = is::to_tensor(cv1);

  is::vision::FrameTransformation tf6;
  tf6.set_from(1001);
  tf6.set_to(1004);
  *tf6.mutable_tf() = is::to_tensor(cv1);

  conversions.update_transformation(tf4);
  conversions.update_transformation(tf5);
  conversions.update_transformation(tf6);

  path = conversions.find_path(is::Path{1000, 2, 1004});
  ASSERT_TRUE(path);
  expected_path = is::Path{1000, 2, 1001, 1004};
  ASSERT_EQ(*path, expected_path);
}

}  // namespace