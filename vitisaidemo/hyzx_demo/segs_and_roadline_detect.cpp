/*
 * Copyright 2019 Xilinx Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * to figure out:
 * 1) input <-- from cv
 * 2) parameters adjustment
 * add channel
 * delete channel
 * adjust layout
 * use drm
 * adjust frame rate (to be done)
 * (raise FPN frame rate, and reduce VPGNET frame rate, to meet 30fps)
 * https://www.xilinx.com/html_docs/vitis_ai/1_3/usz1565818701901.html
 * 3) output --> to cv
 * 4) build script
 * 5) overlay ssd result on segmentation result
 * reference:
 * https://www.xilinx.com/html_docs/vitis_ai/1_3/multitask.html
 * https://github.com/Xilinx/Vitis-AI
 */
#include <glog/logging.h>

#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <vitis/ai/demo.hpp>
#include <vitis/ai/lanedetect.hpp>
#include <vitis/ai/nnpp/lanedetect.hpp>
#include <vitis/ai/multitask.hpp>
#include <vitis/ai/nnpp/multitask.hpp>
#include <vitis/ai/ssd.hpp>
#include <vitis/ai/nnpp/ssd.hpp>
#include <vitis/ai/segmentation.hpp>
#include <vitis/ai/nnpp/segmentation.hpp>
#include <vitis/ai/nnpp/yolov3.hpp>
#include <vitis/ai/yolov3.hpp>

extern std::vector<cv::Rect> GLOBAL_SET_RECT_MANUAL;

#ifndef HAVE_EIGEN
#define HAVE_EIGEN 0
#endif
#if HAVE_EIGEN
#include <eigen3/Eigen/Dense>
// Overlay the original image with the result
// Eigen Optimized version
static void overLay1(cv::Mat &src1, const cv::Mat &src2) {
  const int imsize = src1.cols * src2.rows * 3;
  Eigen::Map<Eigen::Matrix<uchar, -1, 1>> data1(const_cast<uchar *>(src1.data),
                                                imsize);
  Eigen::Map<Eigen::Matrix<uchar, -1, 1>> data2(const_cast<uchar *>(src2.data),
                                                imsize);
  data1 = data1 / 2 + data2 / 2;
}
#else
// c version
static void overLay1(cv::Mat &src1, const cv::Mat &src2) {
  const int imsize = src1.cols * src2.rows * 3;
  for (int i = 0; i < imsize; ++i) {
    src1.data[i] = src1.data[i] / 2 + src2.data[i] / 2;
  }
}
#endif

/*
 *   The color loops every 27 times,
 *    because each channel of RGB loop in sequence of "0, 127, 254"
 */
static cv::Scalar getColor(int label) {
  int c[3];
  for (int i = 1, j = 0; i <= 9; i *= 3, j++) {
    c[j] = ((label / i) % 3) * 127;
  }
  return cv::Scalar(c[2], c[1], c[0]);
}

// This function is used to process the multitask result and show on the image
static cv::Mat process_result_multitask(
    cv::Mat &m1, const vitis::ai::MultiTaskResult &result, bool is_jpeg) {
  (void)process_result_multitask;
  cv::Mat image;
  // Overlay segmentation result to the original image
  if (false) {
    cv::resize(m1, image, result.segmentation.size());
    overLay1(image, result.segmentation);
  } else {
    cv::resize(result.segmentation, image, m1.size());
    overLay1(image, m1);
  }
  // Draw detection results
//  for (auto &r : result.vehicle) {
//    LOG_IF(INFO, is_jpeg) << r.label << " " << r.x << " " << r.y << " "
//                          << r.width << " " << r.height << " " << r.angle;
//    int xmin = r.x * image.cols;
//    int ymin = r.y * image.rows;
//
//    int width = r.width * image.cols;
//    int height = r.height * image.rows;
//    cv::rectangle(image, cv::Rect_<int>(xmin, ymin, width, height),
//                  cv::Scalar(185, 181, 178), 2, 1, 0);
//  }
  return image;
}

using namespace cv;

// This function is used to process the roadline result and show on the image
cv::Mat process_result_roadline(cv::Mat &image,
                                const vitis::ai::RoadLineResult &result,
                                bool is_jpeg) {
  std::vector<int> color1 = {0, 255, 0, 0, 100, 255};
  std::vector<int> color2 = {0, 0, 255, 0, 100, 255};
  std::vector<int> color3 = {0, 0, 0, 255, 100, 255};

  LOG_IF(INFO, is_jpeg) << "lines.size " << result.lines.size() << " ";
  for (auto &line : result.lines) {
    LOG_IF(INFO, is_jpeg) << "line.points_cluster.size() "
                          << line.points_cluster.size() << " ";
    std::vector<cv::Point> points_poly = line.points_cluster;
    int type = line.type < 5 ? line.type : 5;
    if (type == 2 && points_poly[0].x < image.rows * 0.5) continue;
    cv::polylines(image, points_poly, false,
                  cv::Scalar(color1[type], color2[type], color3[type]), 3,
                  CV_AA, 0);
  }
  return image;
}

static cv::Mat process_result_segmentation(cv::Mat &m1,
                              const vitis::ai::SegmentationResult &result,
                              bool is_jpeg) {
  cv::Mat image;
  cv::resize(result.segmentation, image, m1.size());
  overLay1(image, m1);
  //cv::resize(m1, image, result.segmentation.size());
  //cv::Mat image2=image;
//  for (auto row_ind = 0; row_ind < image.size().height; ++row_ind) {
//    for (auto col_ind = 0; col_ind < image.size().width; ++col_ind) {
//      image.at<cv::Vec3b>(row_ind, col_ind) =
//          image.at<cv::Vec3b>(row_ind, col_ind) * 0.5 +
//          result.segmentation.at<cv::Vec3b>(row_ind, col_ind) * 0.5;
//    }
//  }

  return image;

  //Obtain result2 from original image2, Draw detection results on segmentation image
//  auto ssd = vitis::ai::SSD::create("ssd_traffic_pruned_0_9",true);
//  auto results2 = ssd->run(image2);
//
//  for (auto &box : results2.bboxes) {
//    int label = box.label;
//
//    float fxmin = box.x * image2.cols;
//    float fymin = box.y * image2.rows;
//    float fxmax = fxmin + box.width * image2.cols;
//    float fymax = fymin + box.height * image2.rows;
//    float confidence = box.score;
//
//    int xmin = round(fxmin * 100.0) / 100.0;
//    int ymin = round(fymin * 100.0) / 100.0;
//    int xmax = round(fxmax * 100.0) / 100.0;
//    int ymax = round(fymax * 100.0) / 100.0;
//
//    xmin = std::min(std::max(xmin, 0), image2.cols);
//    xmax = std::min(std::max(xmax, 0), image2.cols);
//    ymin = std::min(std::max(ymin, 0), image2.rows);
//    ymax = std::min(std::max(ymax, 0), image2.rows);
//
//    //cout << "RESULT: " << label << "\t" << xmin << "\t" << ymin << "\t" << xmax
//    //     << "\t" << ymax << "\t" << confidence << "\n";
//    if (label == 1) {
//      rectangle(image, Point(xmin, ymin), Point(xmax, ymax), Scalar(0, 255, 0), 1,
//                1, 0);
//    } else if (label == 2) {
//      rectangle(image, Point(xmin, ymin), Point(xmax, ymax), Scalar(255, 0, 0), 1,
//                1, 0);
//    } else if (label == 3) {
//      rectangle(image, Point(xmin, ymin), Point(xmax, ymax), Scalar(0, 0, 255), 1,
//                1, 0);
//    } else if (label == 4) {
//      rectangle(image, Point(xmin, ymin), Point(xmax, ymax), Scalar(0, 255, 255),
//                1, 1, 0);
//    }
//  }

//  auto yolo = vitis::ai::YOLOv3::create("yolov3_adas_pruned_0_9");
//  auto result2 = yolo->run(image2);
//  for (const auto bbox : result2.bboxes) {
//    int label = bbox.label;
//    float xmin = bbox.x * image2.cols + 1;
//    float ymin = bbox.y * image2.rows + 1;
//    float xmax = xmin + bbox.width * image2.cols;
//    float ymax = ymin + bbox.height * image2.rows;
//    float confidence = bbox.score;
//    if (xmax > image2.cols) xmax = image2.cols;
//    if (ymax > image2.rows) ymax = image2.rows;
//
//    if (label == 0) {
//    	cv::rectangle(image, cv::Point(xmin, ymin), cv::Point(xmax, ymax), cv::Scalar(0, 255, 0),
//                1, 1, 0);
//    } else if (label == 1) {
//    	cv::rectangle(image, cv::Point(xmin, ymin), cv::Point(xmax, ymax), cv::Scalar(255, 0, 0),
//                1, 1, 0);
//    } else if (label == 2) {
//    	cv::rectangle(image, cv::Point(xmin, ymin), cv::Point(xmax, ymax), cv::Scalar(0, 0, 255),
//                1, 1, 0);
//    } else if (label == 3) {
//    	cv::rectangle(image, cv::Point(xmin, ymin), cv::Point(xmax, ymax), cv::Scalar(0, 255, 255),
//                1, 1, 0);
//    } else {
//        cv::rectangle(image, cv::Point(xmin, ymin), cv::Point(xmax, ymax), getColor(label), 1, 1, 0);
//    }
//  }
//
//  return image;
}

static cv::Mat process_result_yolov3(cv::Mat &image,
                              const vitis::ai::YOLOv3Result &result,
                              bool is_jpeg) {
  //cv::Mat image2=image;

  for (const auto bbox : result.bboxes) {
    int label = bbox.label;
    float xmin = bbox.x * image.cols + 1;
    float ymin = bbox.y * image.rows + 1;
    float xmax = xmin + bbox.width * image.cols;
    float ymax = ymin + bbox.height * image.rows;
    float confidence = bbox.score;
    if (xmax > image.cols) xmax = image.cols;
    if (ymax > image.rows) ymax = image.rows;
    LOG_IF(INFO, is_jpeg) << "RESULT: " << label << "\t" << xmin << "\t" << ymin
                          << "\t" << xmax << "\t" << ymax << "\t" << confidence
                          << "\n";
    //cv::rectangle(image, cv::Point(xmin, ymin), cv::Point(xmax, ymax),
    //              getColor(label), 1, 1, 0);
    if (label == 0) {
    	cv::rectangle(image, cv::Point(xmin, ymin), cv::Point(xmax, ymax), cv::Scalar(0, 255, 0),
                1, 1, 0);
    } else if (label == 1) {
    	cv::rectangle(image, cv::Point(xmin, ymin), cv::Point(xmax, ymax), cv::Scalar(255, 0, 0),
                1, 1, 0);
    } else if (label == 2) {
    	cv::rectangle(image, cv::Point(xmin, ymin), cv::Point(xmax, ymax), cv::Scalar(0, 0, 255),
                1, 1, 0);
    } else if (label == 3) {
    	cv::rectangle(image, cv::Point(xmin, ymin), cv::Point(xmax, ymax), cv::Scalar(0, 255, 255),
                1, 1, 0);
    } else {
        cv::rectangle(image, cv::Point(xmin, ymin), cv::Point(xmax, ymax), getColor(label), 1, 1, 0);
    }
  }

  //auto det = vitis::ai::RoadLine::create("vpgnet_pruned_0_99");
  //vitis::ai::RoadLineResult result2 = det->run(image);

  //return process_result_roadline(image, result2, false);

  return image;

}

int main(int argc, char *argv[]) {
  // set the layout
  //
//  int seg_px = 100;
//  int seg_py = 252;
	int seg_px = 0;
	int seg_py = 252;
  // assign to Lvalue : static std::vector<cv::Rect> rects, the coordinates of
  // each window
//  gui_layout() = {{seg_px, seg_py, 512, 288},
//                  {seg_px + 512, seg_py, 512, 288},
//                  {seg_px, seg_py + 288, 512, 288},
//                  {seg_px + 512, seg_py + 288, 512, 288},
//                  {1200, 252, 640, 480}};
//  gui_layout() = {{seg_px, seg_py, 640, 480},
//                  {seg_px + 640, seg_py, 640, 480},
//                  {seg_px, seg_py + 480, 512, 480},
//                  {seg_px + 640, seg_py + 480, 640, 480},
//                  {1400, 480, 640, 480}};
//  gui_layout() = {{seg_px, seg_py, 640, 360},
//                  {seg_px + 640, seg_py, 640, 360},
//                  {seg_px, seg_py + 360, 512, 288},
//                  {seg_px + 640, seg_py + 360, 512, 288},
//                  {seg_px+640+640, seg_py, 640, 360},
//  	  	  	  	  {seg_px+640+640, seg_py + 360, 640, 360}};
	  gui_layout() = {{seg_px, seg_py, 640, 360},
	                  {seg_px + 640, seg_py, 640, 360},
	                  {seg_px, seg_py + 360, 640, 360},
	                  {seg_px + 640, seg_py + 360, 640, 360},
	                  {seg_px+640+640, seg_py, 640, 360},
	  	  	  	  	  {seg_px+640+640, seg_py + 360, 640, 360}};
  // assign to Lvalue : set background image
  //
  gui_background() = cv::imread("/usr/share/weston/logo.jpg");
  // init each dpu filter and process instance, using video demo framework
  return vitis::ai::main_for_video_demo_multiple_channel(
      argc, argv,
      {
          [] {
            return vitis::ai::create_dpu_filter(
                [] { return vitis::ai::YOLOv3::create("yolov3_adas_pruned_0_9"); },
				process_result_yolov3);
          },
          [] {
            return vitis::ai::create_dpu_filter(
                [] { return vitis::ai::YOLOv3::create("yolov3_adas_pruned_0_9"); },
				process_result_yolov3);
          },
//          [] {
//            return vitis::ai::create_dpu_filter(
//                [] { return vitis::ai::YOLOv3::create("yolov3_adas_pruned_0_9"); },
//				process_result_yolov3);
//          },
//          [] {
//            return vitis::ai::create_dpu_filter(
//                [] { return vitis::ai::YOLOv3::create("yolov3_adas_pruned_0_9"); },
//				process_result_yolov3);
//          },

//          [] {
//            return vitis::ai::create_dpu_filter(
//                [] { return vitis::ai::MultiTask8UC3::create("multi_task"); },
//                process_result_multitask);
//          },
//          [] {
//            return vitis::ai::create_dpu_filter(
//                [] { return vitis::ai::MultiTask8UC3::create("multi_task"); },
//                process_result_multitask);
//          },
//          [] {
//            return vitis::ai::create_dpu_filter(
//                [] { return vitis::ai::MultiTask8UC3::create("multi_task"); },
//                process_result_multitask);
//          },
//          [] {
//            return vitis::ai::create_dpu_filter(
//                [] { return vitis::ai::MultiTask8UC3::create("multi_task"); },
//                process_result_multitask);
//          },

          [] {
            return vitis::ai::create_dpu_filter(
                [] {
                  return vitis::ai::Segmentation8UC3::create("fpn");
                },
				process_result_segmentation);
          },
          [] {
            return vitis::ai::create_dpu_filter(
                [] {
                  return vitis::ai::Segmentation8UC3::create("fpn");
                },
				process_result_segmentation);
          },

          [] {
            return vitis::ai::create_dpu_filter(
                [] {
                  return vitis::ai::RoadLine::create("vpgnet_pruned_0_99");
                },
                process_result_roadline);
          },
          [] {
            return vitis::ai::create_dpu_filter(
                [] {
                  return vitis::ai::RoadLine::create("vpgnet_pruned_0_99");
                },
                process_result_roadline);
          },
      });
}
