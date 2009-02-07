/*
 * Copyright (c) 2008 Radu Bogdan Rusu <rusu -=- cs.tum.edu>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 *
 */

/** \author Radu Bogdan Rusu */

#ifndef _CLOUD_GEOMETRY_POINT_H_
#define _CLOUD_GEOMETRY_POINT_H_

// ROS includes
#include <std_msgs/PointCloud.h>
#include <std_msgs/Polygon3D.h>
#include <std_msgs/Point32.h>
#include <std_msgs/Point.h>
#include <math.h>

#include <Eigen/Core>

#include <cfloat>

namespace cloud_geometry
{

  /** \brief Simple leaf (3d box) structure) holding a centroid and the number of points in the leaf */
  struct Leaf
  {
    float centroid_x, centroid_y, centroid_z;
    unsigned short nr_points;
  };


  int getChannelIndex (std_msgs::PointCloud *points, std::string channel_name);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief Create a quick copy of a point and its associated channels and return the data as a float vector.
    * \param points the point cloud data message
    * \param index the index of the point to return
    * \param array the vector containing the result
    */
  inline void
    getPointAsFloatArray (std_msgs::PointCloud points, int index, std::vector<float> &array)
  {
    // Resize for XYZ (3) + NR_CHANNELS
    array.resize (3 + points.get_chan_size ());
    array[0] = points.pts[index].x;
    array[1] = points.pts[index].y;
    array[2] = points.pts[index].z;

    for (unsigned int d = 0; d < points.get_chan_size (); d++)
      array[3 + d] = points.chan[d].vals[index];
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief Create a quick copy of a point and its associated channels and return the data as a float vector.
    * \param points the point cloud data message
    * \param index the index of the point to return
    * \param array the vector containing the result
    * \param nr_channels the number of channels to copy (starting with the first channel)
    */
  inline void
    getPointAsFloatArray (std_msgs::PointCloud points, int index, std::vector<float> &array, int nr_channels)
  {
    // Resize for XYZ (3) + NR_CHANNELS
    array.resize (3 + nr_channels);
    array[0] = points.pts[index].x;
    array[1] = points.pts[index].y;
    array[2] = points.pts[index].z;

    for (int d = 0; d < nr_channels; d++)
      array[3 + d] = points.chan[d].vals[index];
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief Create a quick copy of a point and its associated channels and return the data as a float vector.
    * \param points the point cloud data message
    * \param index the index of the point to return
    * \param array the vector containing the result
    * \param start_channel the first channel to start copying data from
    * \param end_channel the last channel to stop copying data at
    */
  inline void
    getPointAsFloatArray (std_msgs::PointCloud points, int index, std::vector<float> &array, int start_channel, int end_channel)
  {
    // Resize for XYZ (3) + NR_CHANNELS
    array.resize (3 + end_channel - start_channel);
    array[0] = points.pts[index].x;
    array[1] = points.pts[index].y;
    array[2] = points.pts[index].z;

    for (int d = start_channel; d < end_channel; d++)
      array[3 + d - start_channel] = points.chan[d].vals[index];
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief Create a quick copy of a point and its associated channels and return the data as a float vector.
    * \param points the point cloud data message
    * \param index the index of the point to return
    * \param array the vector containing the result
    * \param start_channel the first channel to start copying data from
    * \param end_channel the last channel to stop copying data at
    */
  inline void
    getPointAsFloatArray (std_msgs::PointCloud points, int index, std::vector<float> &array, std::vector<int> channels)
  {
    if (channels.size () > points.get_chan_size ())
      return;
    // Resize for XYZ (3) + NR_CHANNELS
    array.resize (3 + channels.size ());
    array[0] = points.pts[index].x;
    array[1] = points.pts[index].y;
    array[2] = points.pts[index].z;

    for (unsigned int d = 0; d < channels.size (); d++)
      array[3 + d] = points.chan[channels.at (d)].vals[index];
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief Compute the cross product between two points (vectors).
    * \param p1 the first point/vector
    * \param p2 the second point/vector
    */
  inline std_msgs::Point32
    cross (std_msgs::Point32 p1, std_msgs::Point32 p2)
  {
    std_msgs::Point32 r;
    r.x = p1.y*p2.z - p1.z*p2.y;
    r.y = p1.z*p2.x - p1.x*p2.z;
    r.z = p1.x*p2.y - p1.y*p2.x;
    return (r);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief Compute the dot product between two points (vectors).
    * \param p1 the first point/vector
    * \param p2 the second point/vector
    */
  inline double
    dot (std_msgs::Point32 p1, std_msgs::Point32 p2)
  {
    return (p1.x * p2.x + p1.y * p2.y + p1.z * p2.z);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief Compute the dot product between two points (vectors).
    * \param p1 the first point/vector
    * \param p2 the second point/vector
    */
  inline double
    dot (std::vector<double> *p1, std::vector<double> *p2)
  {
    return (p1->at (0) * p2->at (0) + p1->at (1) * p2->at (1) + p1->at (2) * p2->at (2));
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief Normalize a point.
    * \param p the point/vector to normalize
    * \param q the resulted normalized point/vector
    */
  inline void
    normalizePoint (std_msgs::Point32 p, std_msgs::Point32 &q)
  {
    // Calculate the 2-norm: norm (x) = sqrt (sum (abs (v)^2))
    double n_norm = sqrt (p.x * p.x + p.y * p.y + p.z * p.z);
    q.x = p.x / n_norm;
    q.y = p.y / n_norm;
    q.z = p.z / n_norm;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief Normalize a point and return the result in place.
    * \param p the point/vector to normalize
    */
  inline void
    normalizePoint (std_msgs::Point32 &p)
  {
    normalizePoint (p, p);
  }

  void getPointIndicesAxisParallelNormals (std_msgs::PointCloud *points, int nx, int ny, int nz, double eps_angle,
                                           std_msgs::Point32 axis, std::vector<int> &indices);
  void getPointIndicesAxisPerpendicularNormals (std_msgs::PointCloud *points, int nx, int ny, int nz, double eps_angle,
                                                std_msgs::Point32 axis, std::vector<int> &indices);

  void downsamplePointCloud (std_msgs::PointCloud *points, std::vector<int> *indices, std_msgs::PointCloud &points_down, std_msgs::Point leaf_size,
                             std::vector<Leaf> &leaves, int d_idx, double cut_distance = DBL_MAX);

  void downsamplePointCloud (std_msgs::PointCloud *points, std_msgs::PointCloud &points_down, std_msgs::Point leaf_size,
                             std::vector<Leaf> &leaves, int d_idx, double cut_distance = DBL_MAX);

  void downsamplePointCloud (std_msgs::PointCloud *points, std_msgs::PointCloud &points_down, std_msgs::Point leaf_size);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief Compute the angle in the [ 0, 2*PI ) interval of a point (direction) with a reference (0, 0) in 2D.
    * \param point a 2D point
    */
  inline double
    getAngle2D (double point[2])
  {
    double rad;
    if (point[0] == 0)
      rad = (point[1] < 0) ? -M_PI / 2.0 : M_PI / 2.0;
    else
    {
      rad = atan (point[1] / point[0]);
      if (point[0] < 0)
        rad += M_PI;
    }
    if (rad < 0)
      rad += 2 * M_PI;

    return (rad);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief Get a u-v-n coordinate system that lies on a plane defined by its normal
    * \param plane_coeff the plane coefficients (containing n, the plane normal)
    * \param u the resultant u direction
    * \param v the resultant v direction
    */
  inline void
    getCoordinateSystemOnPlane (std::vector<double> *plane_coeff, Eigen::Vector3d &u, Eigen::Vector3d &v)
  {
    // Initialize normalized u vector with "random" values not parallel with normal
    u (1) = 0;
    if (fabs (plane_coeff->at (0)) != 1)
    {
      u (0) = 1;
      u (2) = 0;
    }
    else
    {
      u (0) = 0;
      u (2) = 1;
    }

    // Compute the v vector and normalize it
    v (0) = plane_coeff->at (1) * u (2) - plane_coeff->at (2) * u (1);
    v (1) = plane_coeff->at (2) * u (0) - plane_coeff->at (0) * u (2);
    v (2) = plane_coeff->at (0) * u (1) - plane_coeff->at (1) * u (0);
    double v_length = sqrt (v (0) * v (0) + v (1) * v (1) + v (2) * v (2));
    v (0) /= v_length;
    v (1) /= v_length;
    v (2) /= v_length;

    // Recompute u and normalize it
    u (0) = v (1) * plane_coeff->at (2) - v (2) * plane_coeff->at (1);
    u (1) = v (2) * plane_coeff->at (0) - v (0) * plane_coeff->at (2);
    u (2) = v (0) * plane_coeff->at (1) - v (1) * plane_coeff->at (0);
    double u_length = sqrt (u (0) * u (0) + u (1) * u (1) + u (2) * u (2));
    u (0) /= u_length;
    u (1) /= u_length;
    u (2) /= u_length;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief Write the point data to screen (stderr)
    * \param p the point
    */
  inline void
    cerr_p (std_msgs::Point32 p)
  {
    std::cerr << p.x << " " << p.y << " " << p.z << std::endl;
  }

}

#endif
