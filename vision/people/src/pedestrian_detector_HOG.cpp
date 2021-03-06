/*********************************************************************
 *
 * Software License Agreement (BSD License)
 * 
 *  Copyright (c) 2008, Willow Garage, Inc
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

#include <stdio.h>
#include <iostream>
#include <stdint.h>

#include "ros/node.h"
#include "ros/console.h"
#include "CvStereoCamModel.h"
#include <people/PositionMeasurement.h>
#include "stereo_msgs/StereoInfo.h"
#include "stereo_msgs/DisparityInfo.h"
#include "sensor_msgs/CameraInfo.h"
#include "sensor_msgs/Image.h"
#include "opencv_latest/CvBridge.h"
#include "people/ColoredLines.h"
#include "topic_synchronizer/topic_synchronizer.h"
#include "tf/transform_listener.h"
#include <tf/message_notifier.h>
#include "LinearMath/btTransform.h"

#include "opencv/cxcore.h"
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include <boost/thread/mutex.hpp>
#include "opencv/cvaux.hpp"

#include "utils.h"

//#include "HOG/hog.hpp"

namespace people
{

  bool DEBUG_DISPLAY = false;

  using namespace std;

  // PedestrianDetectorHOG - a wrapper around the Dalal-Triggs HOG pedestrian detector in OpenCV, plus the use of 3d information.

  class PedestrianDetectorHOG{
  public:

    // ROS
    ros::Node *node_;

    // Images and conversion
    sensor_msgs::Image limage_;
    sensor_msgs::Image dimage_;
    stereo_msgs::StereoInfo stinfo_;
    stereo_msgs::DisparityInfo dispinfo_;
    sensor_msgs::CameraInfo rcinfo_;
    sensor_msgs::CvBridge lbridge_;
    sensor_msgs::CvBridge dbridge_;
    TopicSynchronizer<PedestrianDetectorHOG> *sync_;

    tf::TransformListener *tf_;

    double hit_threshold_;
    int group_threshold_;

    bool use_depth_;

    bool use_height_;
    double max_height_m_;
    std::string ground_frame_;

    bool do_display_;

    /////////////////////////////////////////////////////////////////
    // Constructor
    PedestrianDetectorHOG(ros::Node *node): node_(node), cam_model_(NULL), counter(0) {
      
      hog_.setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector());
      
      // Get parameters from the server
      node_->param("/people/pedestrian_detector_HOG/hit_threshold",hit_threshold_,0.0);
      node_->param("/people/pedestrian_detector_HOG/group_threshold",group_threshold_,2);
      node_->param("/people/pedestrian_detector_HOG/use_depth", use_depth_, true);
      node_->param("/people/pedestrian_detector_HOG/use_height",use_height_,true);
      node_->param("/people/pedestrian_detector_HOG/max_height_m",max_height_m_,2.2);
      node_->param("/people/pedestrian_detector_HOG/ground_frame",ground_frame_,std::string("base_link"));
      node_->param("/people/pedestrian_detector_HOG/do_display", do_display_, true);
       
      // Advertise a 3d position measurement for each head.
      node_->advertise<people::PositionMeasurement>("people_tracker_measurements",1);

      // Advertise the display boxes.
      if (do_display_) {
	node_->advertise<people::ColoredLines>("lines_to_draw",1);
	ROS_INFO_STREAM_NAMED("pedestrian_detector_HOG","Advertising colored lines to draw remotely.");
	//cv::namedWindow("people detector", 1);
      }

      // Subscribe to tf & the images  
      if (use_depth_) {  

	tf_ = new tf::TransformListener(*node_);
	tf_->setExtrapolationLimit(ros::Duration().fromSec(0.01));

	sync_ = new TopicSynchronizer<PedestrianDetectorHOG>(node_, this, &people::PedestrianDetectorHOG::imageCBAll, ros::Duration().fromSec(0.05), &PedestrianDetectorHOG::imageCBTimeout);
	sync_->subscribe("stereo/left/image_rect",limage_,1);
	sync_->subscribe("stereo/disparity",dimage_,1);
	sync_->subscribe("stereo/stereo_info",stinfo_,1);
	sync_->subscribe("stereo/disparity_info",dispinfo_,1);
	sync_->subscribe("stereo/right/cam_info",rcinfo_,1);
	sync_->ready();
      }
      else {
	node_->subscribe("stereo/left/image_rect",limage_,&people::PedestrianDetectorHOG::imageCB,this,1);
      }

    }
    

    /////////////////////////////////////////////////////////////////
    // Destructor
    ~PedestrianDetectorHOG(){
    }

    
    /////////////////////////////////////////////////////////////////////
    // The image callback when not all topics are sync'ed. Don't do anything, just wait for sync.
    void imageCBTimeout(ros::Time t) {
      ROS_DEBUG_STREAM_NAMED("pedestrian_detector_HOG","Message timeout");
    }

    /////////////////////////////////////////////////////////////////////
    // Image callback for an image without stereo, etc.
    void imageCB() {
      imageCBAll(limage_.header.stamp);
    }


    /////////////////////////////////////////////////////////////////////
    // Image callback
    void imageCBAll(ros::Time t)
    {
      // Convert the image to OpenCV
      if (!lbridge_.fromImage(limage_,"mono8")) 
	return;
      IplImage *cv_image_left = lbridge_.toIpl();

      IplImage *cv_image_disp = NULL;
      if (use_depth_) {
	if (!dbridge_.fromImage(dimage_)) 
	  return;
	cv_image_disp = dbridge_.toIpl();
      }

      // TODO: Finish this function
      
      // TODO: Preprocess the image for the detector

      cv::Mat img(cv_image_left,false);
      cv::Mat *small_img;
      cv::Vector<cv::Rect> found;
      if (!use_depth_) {

	// Timing
	struct timeval timeofday;
	gettimeofday(&timeofday,NULL);
	ros::Time startt = ros::Time().fromNSec(1e9*timeofday.tv_sec + 1e3*timeofday.tv_usec);


	// Run the HOG detector. Similar to samples/peopledetect.cpp.
	hog_.detectMultiScale(img, found, hit_threshold_, cv::Size(8,8), cv::Size(24,16), 1.05, group_threshold_);
	small_img = &img;

	// Timing
	gettimeofday(&timeofday,NULL);
	ros::Time endt = ros::Time().fromNSec(1e9*timeofday.tv_sec + 1e3*timeofday.tv_usec);
	ros::Duration diff = endt-startt;
	ROS_DEBUG_STREAM_NAMED("pedestrian_detector","No depth, duration " << diff.toSec() );

      }
      else {

	// Convert the stereo calibration into a camera model. Only done once.
	if (!cam_model_) {
	  double Fx = rcinfo_.P[0];
	  double Fy = rcinfo_.P[5];
	  double Clx = rcinfo_.P[2];
	  double Crx = Clx;
	  double Cy = rcinfo_.P[6];
	  double Tx = -rcinfo_.P[3]/Fx;
	  cam_model_ = new CvStereoCamModel(Fx,Fy,Tx,Clx,Crx,Cy,1.0/dispinfo_.dpp);
	}

	// Timing
	struct timeval timeofday;
	gettimeofday(&timeofday,NULL);
	ros::Time startt = ros::Time().fromNSec(1e9*timeofday.tv_sec + 1e3*timeofday.tv_usec);

	////////////////////////////////
	// Get a 3d point for each pixel

	CvMat *uvd = cvCreateMat(img.cols*img.rows,3,CV_32FC1);
	CvMat *xyz = cvCreateMat(img.cols*img.rows,3,CV_32FC1);
	// Convert image uvd to world xyz.
	float *fptr = (float*)(uvd->data.ptr);
	ushort *cptr;
	for (int v =0; v < img.rows; ++v) {
	  cptr = (ushort*)(cv_image_disp->imageData+v*cv_image_disp->widthStep);
	  for (int u=0; u<img.cols; ++u) {
	    (*fptr) = (float)u; ++fptr;
	    (*fptr) = (float)v; ++fptr;
	    (*fptr) = (float)(*cptr); ++fptr; ++cptr;      
	  }
	}
	cam_model_->dispToCart(uvd,xyz);
	//CvMat *y = cvCreateMat(img.cols*img.rows,1,CV_32FC1);
	//CvMat *z = cvCreateMat(img.cols*img.rows,1,CV_32FC1);
	//cvSplit(xyz, NULL, y, z, NULL);
	cv::Mat xyzmat(xyz,false);
	//cv::Mat zmap(z,false);

	if (use_height_) {
	  // Remove the ceiling
	  int top_row;
	  removeCeiling(img, xyzmat, limage_.header.frame_id, limage_.header.stamp, max_height_m_, top_row);
	  small_img = new cv::Mat(img, cv::Rect(cv::Point(0, top_row), cv::Size(img.cols, img.rows-top_row)));
	}
	else {
	  small_img = &img;
	}

	hog_.detectMultiScale(*small_img, found, hit_threshold_, cv::Size(8,8), cv::Size(24,16), 1.05, group_threshold_);
	
	//	cvReleaseMat(&z);
	cvReleaseMat(&uvd);
	cvReleaseMat(&xyz);
	
	// Timing
	gettimeofday(&timeofday,NULL);
	ros::Time endt = ros::Time().fromNSec(1e9*timeofday.tv_sec + 1e3*timeofday.tv_usec);
	ros::Duration diff = endt-startt;
	ROS_DEBUG_STREAM_NAMED("pedestrian_detector","Remove ceiling, duration " << diff.toSec() );
	
      }
      
      // TODO: Publish the found people.

      for( int i = 0; i < (int)found.size(); i++ )
      {
	cv::Rect r = found[i];
	cv::rectangle(*small_img, r.tl(), r.br(), cv::Scalar(0,255,0), 1);
      }
      ostringstream fname;
      fname << "/tmp/left_HOG_OpenCV_results2/" << setfill('0') << setw(6) << counter << "L.jpg";
      counter ++;
      cv::imwrite(fname.str(), *small_img);
      //cv::imshow("people detector", img);
      //cv::waitKey(3);

      // TODO: Release the sequence and related memory.
      
    }


  private:

    CvStereoCamModel *cam_model_;
    cv::HOGDescriptor hog_;

    int counter;

    /////////////////////////////////////////////////////////////////////
    // Remove the ceiling (rows above max_height_m_)
    // For each row, compute the maximum robot-relative height.
    // Find the first row with z-values below the max_height_m_ threshold.
    // Return the row for image cropping.
    void removeCeiling(cv::Mat &img, const cv::Mat &xyz, string img_frame, ros::Time time, double max_height_m_, int &top_row) 
    {
      // Get the rotation and translation matrices for converting the camera "world" coords to the "real" world.
      // Note that a height of 0m in the "real" world is at the ground_frame_ origin. 
      tf::Stamped<btTransform> transform_im_pt_to_ground;
      tf_->lookupTransform(ground_frame_, img_frame, time, transform_im_pt_to_ground);
      btVector3 translation = transform_im_pt_to_ground.getOrigin();
      btMatrix3x3 rotation = transform_im_pt_to_ground.getBasis();
      btVector3 row2 = rotation.getRow(2);

      // Convert points to heights, starting at the top-left of the image. 
      // The image wil be cropped at the highest row to have a "real"-world height of 
      // less than max_height_m_.
      float *ptr = (float*)(xyz.data);
      int npoints = img.rows * img.cols;
      for (int p=0; p<npoints; ++p, ptr+=3) {
	// If invalid depth, continue.
	if (*(ptr+2) == 0.0) 
	  continue;
       
	// Get the robot-relative height. Return if it's too low.
	double z = row2[0]*(*ptr) + row2[1]*(*(ptr+1)) + row2[2]*(*(ptr+2)) + translation[2];
	if (z <= max_height_m_) {
	  top_row = (int)(floor((float)p/(float)img.cols));
	  return;
	}
      }

      top_row = 0;
    }

    /////////////////////////////////////////////////////////////////////
    // TODO: Get possible person positions and scales by considering the floor plane.
    // - Listen to the floor plane
    // - Project the floor plane into the image
    // - If the floor plane is not horizontal to the image, the image should probably be rotated to make the next step just use horizontal bands.
    // - At each pixel in the image, given the depth on the floor plane at that pixel, set the predicted height(s) and position for detection.
    // - Detect and return the people.


    /////////////////////////////////////////////////////////////////////
    // TODO: Get possible person positions and scales from leg detections.
    // - Listen to leg detections.
    // - Synchronize leg detections with this image frame.
    // - Transform the leg detections to the stereo_optical_frame.
    // - Project into the image.
    // - Extract a portion of the image around the detection. 
    // -- Note that the people are only 3/4 of the bbox size (96pix person --> 128pix box)
    // -- How to define the box? Big enough so leg can be at left-most or right-most edge?
    // -- How does this interact with the multiple detections needed at each position? If the box is too tight, will detection fail?
    // - Detect people at that location. 
    // - Return all of the detected people.


    /////////////////////////////////////////////////////////////////////
    // TODO: Get possible person positions and scales from stereo depth.
    // - I'm not sure this is possible with our current stereo setup. There are so many holes that detection will still be expensive.


    /////////////////////////////////////////////////////////////////////
    // TODO: Get possible person positions and scales from laser 3d information.

  }; // Class
}; // Namespace




/////////////////////////////////////////////////////////////////////

int main (int argc, char** argv)
{
  ros::init (argc, argv);

  ros::Node pd("pedestrian_detector_HOG");

  people::PedestrianDetectorHOG pdhog(&pd);

  pd.spin ();
  return (0);
}


