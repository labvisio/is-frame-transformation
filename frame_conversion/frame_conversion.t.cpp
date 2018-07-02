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

  conversions.add(tf1);
  conversions.add(tf2);
  conversions.add(tf3);
  auto composition = conversions.find(1001, 1000);
  ASSERT_TRUE(composition);
  auto ids = std::vector<is::FrameIds>{{1001, 1}, {1, 1000}};
  ASSERT_EQ(composition->ids, ids);
  ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(composition->tf,
                                                                 is::to_tensor(cv1.inv() * cv2)));

  composition = conversions.find(3000, 1000);
  ASSERT_FALSE(composition);
  ASSERT_EQ(composition.error(), "Invalid frame \"3000\"");

  composition = conversions.find(1000, 3000);
  ASSERT_FALSE(composition);
  ASSERT_EQ(composition.error(), "Invalid frame \"3000\"");

  composition = conversions.find(1000, 1002);
  ASSERT_FALSE(composition);
  ASSERT_EQ(composition.error(), "Frames \"1000\" and \"1002\" are not connected");
}

}  // namespace