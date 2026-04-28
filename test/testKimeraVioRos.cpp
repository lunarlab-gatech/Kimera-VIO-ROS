/**
 * @file   testKimeraVioRos.cpp
 * @brief  Unit tests for Kimera-VIO-ROS utilities.
 */

#include <glog/logging.h>
#include <gtest/gtest.h>

#include <opencv2/opencv.hpp>
#include <ros/ros.h>
#include <sensor_msgs/CompressedImage.h>
#include <sensor_msgs/image_encodings.h>

#include "kimera_vio_ros/utils/UtilsRos.h"

namespace VIO {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

sensor_msgs::CompressedImageConstPtr makeCompressedImage(
    const cv::Mat& image,
    const std::string& format = "png") {
  std::vector<uint8_t> buf;
  cv::imencode("." + format, image, buf);

  auto msg = boost::make_shared<sensor_msgs::CompressedImage>();
  msg->header.stamp = ros::Time(1.0);
  msg->header.frame_id = "test_frame";
  msg->format = format;
  msg->data = buf;
  return msg;
}

// Build a 64x64 ramp image where intensity = (r + c) * 2.
// For a 64x64 image the max value is (63 + 63) * 2 = 252, so there is no
// uint8_t overflow and the image is strictly monotonically increasing from
// top-left (0) to bottom-right (252). This lets spatial-ordering tests detect
// any row/column swap or byte reordering.
cv::Mat makeGradientImage() {
  cv::Mat img(64, 64, CV_8UC1);
  for (int r = 0; r < img.rows; ++r) {
    for (int c = 0; c < img.cols; ++c) {
      img.at<uint8_t>(r, c) = static_cast<uint8_t>((r + c) * 2);
    }
  }
  return img;
}

// ---------------------------------------------------------------------------
// decompressImage tests
// ---------------------------------------------------------------------------

TEST(DecompressImage, RoundTripDimensions) {
  const cv::Mat src = makeGradientImage();
  const sensor_msgs::ImagePtr result =
      utils::decompressImage(makeCompressedImage(src));

  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->height, static_cast<uint32_t>(src.rows));
  EXPECT_EQ(result->width, static_cast<uint32_t>(src.cols));
}

TEST(DecompressImage, OutputEncodingIsMono8) {
  const sensor_msgs::ImagePtr result =
      utils::decompressImage(makeCompressedImage(makeGradientImage()));

  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->encoding, sensor_msgs::image_encodings::MONO8);
}

TEST(DecompressImage, StepEqualsWidth) {
  const sensor_msgs::ImagePtr result =
      utils::decompressImage(makeCompressedImage(makeGradientImage()));

  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->step, result->width);
}

TEST(DecompressImage, DataSizeMatchesHeightTimesStep) {
  const sensor_msgs::ImagePtr result =
      utils::decompressImage(makeCompressedImage(makeGradientImage()));

  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->data.size(), result->height * result->step);
}

TEST(DecompressImage, HeaderPreserved) {
  const cv::Mat src = makeGradientImage();
  const auto compressed = makeCompressedImage(src);
  const sensor_msgs::ImagePtr result = utils::decompressImage(compressed);

  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->header.stamp, compressed->header.stamp);
  EXPECT_EQ(result->header.frame_id, compressed->header.frame_id);
}

// PNG is lossless, so decoded pixel values must match exactly. We test a
// spatial gradient so that row/column swaps and byte reorderings are caught.
TEST(DecompressImage, SpatialIntegrityLossless) {
  const cv::Mat src = makeGradientImage();
  const sensor_msgs::ImagePtr result =
      utils::decompressImage(makeCompressedImage(src, "png"));

  ASSERT_NE(result, nullptr);
  ASSERT_EQ(result->data.size(),
            static_cast<size_t>(src.rows) * static_cast<size_t>(src.cols));

  for (int r = 0; r < src.rows; ++r) {
    for (int c = 0; c < src.cols; ++c) {
      const uint8_t expected = src.at<uint8_t>(r, c);
      const uint8_t actual = result->data[r * result->step + c];
      EXPECT_EQ(actual, expected)
          << "Pixel mismatch at row=" << r << " col=" << c;
    }
  }
}

// For JPEG (lossy), verify that relative spatial ordering is preserved:
// pixels that are brighter in the source are brighter in the decoded result.
TEST(DecompressImage, SpatialOrderingLossy) {
  const cv::Mat src = makeGradientImage();
  const sensor_msgs::ImagePtr result =
      utils::decompressImage(makeCompressedImage(src, "jpeg"));

  ASSERT_NE(result, nullptr);

  // Top-left corner (r=0,c=0) should be darker than bottom-right (r=63,c=63).
  const uint8_t top_left = result->data[0];
  const uint8_t bottom_right =
      result->data[(src.rows - 1) * result->step + (src.cols - 1)];
  EXPECT_LT(top_left, bottom_right);

  // Top-right corner should be darker than bottom-right (row gradient).
  const uint8_t top_right = result->data[src.cols - 1];
  EXPECT_LT(top_right, bottom_right);
}

TEST(DecompressImage, ReturnsNullptrOnEmptyData) {
  auto msg = boost::make_shared<sensor_msgs::CompressedImage>();
  msg->header.stamp = ros::Time(0.0);
  msg->format = "jpeg";
  // data left empty — cv::imdecode will return an empty Mat

  const sensor_msgs::ImagePtr result =
      utils::decompressImage(sensor_msgs::CompressedImageConstPtr(msg));
  EXPECT_EQ(result, nullptr);
}

TEST(DecompressImage, ReturnsNullptrOnGarbageData) {
  auto msg = boost::make_shared<sensor_msgs::CompressedImage>();
  msg->header.stamp = ros::Time(0.0);
  msg->format = "jpeg";
  msg->data = {0xDE, 0xAD, 0xBE, 0xEF};

  const sensor_msgs::ImagePtr result =
      utils::decompressImage(sensor_msgs::CompressedImageConstPtr(msg));
  EXPECT_EQ(result, nullptr);
}

}  // namespace VIO

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ros::init(argc, argv, "test_kimera_vio_ros");
  google::InitGoogleLogging(argv[0]);

  FLAGS_logtostderr = true;
  FLAGS_alsologtostderr = true;
  FLAGS_colorlogtostderr = true;

  return RUN_ALL_TESTS();
}
