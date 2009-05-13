/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

//! \author Vijay Pradeep

#ifndef STEREO_CHECKERBOARD_DETECTOR_STEREO_CHECKERBOARD_HELPER_H_
#define STEREO_CHECKERBOARD_DETECTOR_STEREO_CHECKERBOARD_HELPER_H_

#include <vector>

#include "robot_msgs/PoseStamped.h"

#include "opencv/cv.h"
#include "opencv_latest/CvBridge.h"

#include "stereo_checkerboard_detector/mono_checkerboard_helper.h"
#include "stereo_checkerboard_detector/reprojection_helper.h"

namespace stereo_checkerboard_detector
{


/**
 * Performs the basic openCV operations to get checkerboard information from ROS Images from a stereocam pair
 */
class StereoCheckerboardHelper
{
public:

  StereoCheckerboardHelper(int w=2, int h=2)
  {
    setSize(w,h) ;
  }
  ~StereoCheckerboardHelper() { }

  /**
   * Sets the size of the checkerboard we're looking for
   * \param w Num checkerboard squares Wide
   * \param h Num checkerboard squares high
   */
  void setSize(int w, int h)
  {
    board_size_ = cvSize(w,h) ;
    left_helper_.setSize(w,h) ;
    right_helper_.setSize(w,h) ;
  }

  /**
   *
   * \todo const-ify images, once cv_bridge is const correct
   */
  bool findCheckerboard(image_msgs::Image& left, image_msgs::Image& right,
                        const image_msgs::CamInfo& left_info, const image_msgs::CamInfo& right_info) ;


  /**
   *
   */
  void getCorners(std::vector<robot_msgs::Point>& xyz)
  {
    xyz.clear() ;
    xyz = xyz_ ;
  }

  const image_msgs::Image& getLeftDebug()
  {
    return left_ros_debug_ ;
  }

  const image_msgs::Image& getRightDebug()
  {
    return right_ros_debug_ ;
  }


private:
  MonoCheckerboardHelper left_helper_ ;
  MonoCheckerboardHelper right_helper_ ;
  ReprojectionHelper reproj_helper_ ;
  image_msgs::CvBridge left_bridge_ ;
  image_msgs::CvBridge right_bridge_ ;

  image_msgs::Image left_ros_debug_ ;
  image_msgs::Image right_ros_debug_ ;

  //! Stores corner locations cartesian coordinates
  std::vector<robot_msgs::Point> xyz_ ;

  CvSize board_size_ ;                   //!< Size of the checkboard
} ;

}

#endif // CHECKERBOARD_DETECTOR_STEREO_CHECKERBOARD_HELPER_H_
