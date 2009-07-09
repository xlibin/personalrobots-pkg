/*
 * visualization_utils.cpp
 *
 *  Created on: Jul 8, 2009
 *      Author: sturm
 */

#include "vis_utils.h"
#include <point_cloud_mapping/geometry/areas.h>

using namespace ros;
using namespace std;
using namespace robot_msgs;
using namespace vis_utils;

#define RETURN_RGB(r1,g1,b1) r=(int)(r1*255);g=(int)(g1*255);b=(int)(b1*255);


int vis_utils::HSV_to_RGB( float h, float s, float v) {
        // H is given on [0, 6] or UNDEFINED. S and V are given on [0, 1].
        // RGB are each returned on [0, 1].
        h *= 6;
        int i;
        float m,n,f;

        i = floor(h);
        f = h - i;
        if ( !(i&1) ) f = 1 - f; // if i is even
        m = v * (1 - s);
        n = v * (1 - s * f);
        int r,g,b;
        switch (i) {
                case 6:
                case 0: RETURN_RGB(v, n, m); break;
                case 1: RETURN_RGB(n, v, m);break;
                case 2: RETURN_RGB(m, v, n);break;
                case 3: RETURN_RGB(m, n, v);break;
                case 4: RETURN_RGB(n, m, v);break;
                case 5: RETURN_RGB(v, m, n);break;
                default: RETURN_RGB(1,0.5,0.5);
        }
        int rgb;
        rgb=r<<16 | g<<8 | b;
////        rgb=b;
        ROS_INFO("%f %d: %f %f %f; %d %d %d => %d",h,i,v,n,m,r,g,b,rgb);
        return(rgb);
}

float vis_utils::HSV_to_RGBf(float h, float s, float v) {
  int rgb=HSV_to_RGB(h,s,v);
  return(*(float*)&rgb);
}


void vis_utils::visualizePlanes(const robot_msgs::PointCloud& cloud,
                                  std::vector<std::vector<int> >& plane_indices,
                                  std::vector<robot_msgs::PointCloud>& plane_cloud,
                                  std::vector<std::vector<double> >& plane_coeff,
                                  robot_msgs::PointCloud& outside,
                                  ros::Publisher& cloud_planes_pub,ros::Publisher& visualization_pub)
{
  PointCloud colored_cloud = cloud;

  int rgb_chann=-1;
  for(size_t i=0;i<cloud.chan.size();i++)
    if(cloud.chan[i].name=="rgb") rgb_chann=i;

  int rgb;
  if(rgb_chann!=-1) {
    rgb=HSV_to_RGB(0.7,0,0.3);
    for(size_t i=0;i<cloud.pts.size();i++) {
      colored_cloud.chan[rgb_chann].vals[i] = *(float*)&rgb;
    }

    for(size_t i=0;i<plane_indices.size();i++) {
      rgb=HSV_to_RGB( i/(float)plane_indices.size(),0.3,1);
      for(size_t j=0;j<plane_indices[i].size();j++) {
        colored_cloud.chan[rgb_chann].vals[plane_indices[i][j]] = *(float*)&rgb;
      }

      rgb=HSV_to_RGB( i/(float)plane_indices.size(),0.5,1);
      Polygon3D polygon;
      cloud_geometry::areas::convexHull2D(cloud, plane_indices[i], plane_coeff[i],
                      polygon);
      visualizePolygon(cloud, polygon,rgb,i,visualization_pub);



    }
  }

  cloud_planes_pub.publish(colored_cloud);
}

void vis_utils::visualizePolygon(const robot_msgs::PointCloud& cloud,robot_msgs::Polygon3D &polygon, int rgb, int id, ros::Publisher& visualization_pub ) {
  visualization_msgs::Marker marker;
  marker.header.frame_id = cloud.header.frame_id;
  marker.header.stamp = ros::Time((uint64_t)0ULL);
  marker.ns = "polygon";
  marker.id = id;
  marker.type = visualization_msgs::Marker::LINE_STRIP;
  marker.action = visualization_msgs::Marker::ADD;
  marker.pose.orientation.w = 1.0;
  marker.scale.x = 0.002;
  marker.color.a = 1.0;
  marker.color.r = ((rgb >> 16) & 0xff)/255.0;
  marker.color.g = ((rgb >> 8) & 0xff)/255.0;
  marker.color.b = ((rgb >> 0) & 0xff)/255.0;

  marker.set_points_size(polygon.get_points_size());

  for (size_t i=0;i<polygon.get_points_size();++i) {
          marker.points[i].x = polygon.points[i].x;
          marker.points[i].y = polygon.points[i].y;
          marker.points[i].z = polygon.points[i].z;
  }
  ROS_INFO("visualization_marker polygon with n=%d points",polygon.get_points_size());
  visualization_pub.publish(  marker );
}