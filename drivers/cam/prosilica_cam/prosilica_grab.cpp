#include "ros/node.h"
#include "image_msgs/CvBridge.h"

#include "opencv/cv.h"
#include "opencv/highgui.h"

#include "prosilica_cam/PolledImage.h"

static char wndname[] = "Captured image";

image_msgs::CvBridge bridge;

// NOTE: res must be global! CvBridge assumes it can just point
//       to the pixel data in the image message.
prosilica_cam::PolledImage::Request req;
prosilica_cam::PolledImage::Response res;

IplImage* callPollProsilica(int timeout)
{
  req.timeout_ms = timeout;
  if (!ros::service::call("prosilica/poll", req, res)) {
    ROS_FATAL("Service call failed");
    return NULL;
  }
  
  if (!bridge.fromImage(res.image, "bgr")) {
    ROS_FATAL("CvBridge::fromImage failed");
    return NULL;
  }
  
  return bridge.toIpl();
}

int main(int argc, char** argv)
{
  ros::init(argc, argv);
  ros::Node n("poll_prosilica_client");

  cvNamedWindow(wndname, 0); // no autosize

  unsigned int index = 0;
  IplImage* display = NULL;
  char filename[16];
  while (true)
  {
    int k = cvWaitKey(0);
    switch( (char) k)
    {
      case 'q':
        goto exit_main;
      case 'c':
        display = callPollProsilica(100);
        if (display)
          cvShowImage(wndname, display);
        break;
      case 's':
        if (display) {
          sprintf(filename, "frame%04u.jpg", index++);
          cvSaveImage( filename, display );
          ROS_FATAL("Saved image %s\n", filename);
        }
        break;
    }
  }

exit_main:
  cvDestroyWindow(wndname);
}
