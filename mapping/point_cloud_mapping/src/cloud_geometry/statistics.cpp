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

#include <algorithm>
#include <cfloat>
#include <cloud_geometry/statistics.h>

namespace cloud_geometry
{

  namespace statistics
  {
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /** \brief Compute the median value of a 3D point cloud and return it as a Point32.
      * \param points the point cloud data message
      */
    std_msgs::Point32
      computeMedian (std_msgs::PointCloud points)
    {
      std_msgs::Point32 median;

      // Copy the values to vectors for faster sorting
      std::vector<double> x (points.pts.size ());
      std::vector<double> y (points.pts.size ());
      std::vector<double> z (points.pts.size ());
      for (unsigned int i = 0; i < points.pts.size (); i++)
      {
        x[i] = points.pts[i].x;
        y[i] = points.pts[i].y;
        z[i] = points.pts[i].z;
      }
      std::sort (x.begin (), x.end ());
      std::sort (y.begin (), y.end ());
      std::sort (z.begin (), z.end ());

      int mid = points.pts.size () / 2;
      if (points.pts.size () % 2 == 0)
      {
        median.x = (x[mid-1] + x[mid]) / 2;
        median.y = (y[mid-1] + y[mid]) / 2;
        median.z = (z[mid-1] + z[mid]) / 2;
      }
      else
      {
        median.x = x[mid];
        median.y = y[mid];
        median.z = z[mid];
      }
      return (median);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /** \brief Compute the median value of a 3D point cloud using a given set point
      * indices and return it as a Point32.
      * \param points the point cloud data message
      * \param indices the point indices
      */
    std_msgs::Point32
      computeMedian (std_msgs::PointCloud points, std::vector<int> indices)
    {
      std_msgs::Point32 median;

      // Copy the values to vectors for faster sorting
      std::vector<double> x (indices.size ());
      std::vector<double> y (indices.size ());
      std::vector<double> z (indices.size ());
      for (unsigned int i = 0; i < indices.size (); i++)
      {
        x[i] = points.pts.at (indices.at (i)).x;
        y[i] = points.pts.at (indices.at (i)).y;
        z[i] = points.pts.at (indices.at (i)).z;
      }
      std::sort (x.begin (), x.end ());
      std::sort (y.begin (), y.end ());
      std::sort (z.begin (), z.end ());

      int mid = indices.size () / 2;
      if (indices.size () % 2 == 0)
      {
        median.x = (x[mid-1] + x[mid]) / 2;
        median.y = (y[mid-1] + y[mid]) / 2;
        median.z = (z[mid-1] + z[mid]) / 2;
      }
      else
      {
        median.x = x[mid];
        median.y = y[mid];
        median.z = z[mid];
      }
      return (median);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /** \brief Compute the median absolute deviation:
      * \f[
      * MAD = \sigma * median_i (| Xi - median_j(Xj) |)
      * \f]
      * \note Sigma needs to be chosen carefully (a good starting sigma value is 1.4826)
      * \param points the point cloud data message
      * \param sigma the sigma value
      */
    double
      computeMedianAbsoluteDeviation (std_msgs::PointCloud points, double sigma)
    {
      // median (dist (x - median (x)))
      std_msgs::Point32 median = computeMedian (points);

      std::vector<double> distances (points.pts.size ());

      for (unsigned int i = 0; i < points.pts.size (); i++)
        distances[i] = (points.pts[i].x - median.x) * (points.pts[i].x - median.x) +
                       (points.pts[i].y - median.y) * (points.pts[i].y - median.y) +
                       (points.pts[i].z - median.z) * (points.pts[i].z - median.z);

      std::sort (distances.begin (), distances.end ());

      double result;
      int mid = points.pts.size () / 2;
      // Do we have a "middle" point or should we "estimate" one ?
      if (points.pts.size () % 2 == 0)
        result = (sqrt (distances[mid-1]) + sqrt (distances[mid])) / 2;
      else
        result = sqrt (distances[mid]);
      return (sigma * result);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /** \brief Compute the median absolute deviation:
      * \f[
      * MAD = \sigma * median_i (| Xi - median_j(Xj) |)
      * \f]
      * \note Sigma needs to be chosen carefully (a good starting sigma value is 1.4826)
      * \param points the point cloud data message
      * \param sigma the sigma value
      */
    double
      computeMedianAbsoluteDeviation (std_msgs::PointCloud points, std::vector<int> indices, double sigma)
    {
      // median (dist (x - median (x)))
      std_msgs::Point32 median = computeMedian (points, indices);

      std::vector<double> distances (indices.size ());

      for (unsigned int i = 0; i < indices.size (); i++)
        distances[i] = (points.pts.at (indices.at (i)).x - median.x) * (points.pts.at (indices.at (i)).x - median.x) +
                       (points.pts.at (indices.at (i)).y - median.y) * (points.pts.at (indices.at (i)).y - median.y) +
                       (points.pts.at (indices.at (i)).z - median.z) * (points.pts.at (indices.at (i)).z - median.z);

      std::sort (distances.begin (), distances.end ());

      double result;
      int mid = indices.size () / 2;
      // Do we have a "middle" point or should we "estimate" one ?
      if (indices.size () % 2 == 0)
        result = (sqrt (distances[mid-1]) + sqrt (distances[mid])) / 2;
      else
        result = sqrt (distances[mid]);

      return (sigma * result);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /** \brief Compute both the mean and the standard deviation of a channel/dimension of the cloud
      * \param points a pointer to the point cloud message
      * \param d_idx the channel index
      * \param mean the resultant mean of the distribution
      * \param stddev the resultant standard deviation of the distribution
      */
    void
      getChannelMeanStd (std_msgs::PointCloud *points, int d_idx, double &mean, double &stddev)
    {
      double sum = 0, sq_sum = 0;

      for (unsigned int i = 0; i < points->pts.size (); i++)
      {
        sum += points->chan.at (d_idx).vals.at (i);
        sq_sum += points->chan.at (d_idx).vals.at (i) * points->chan.at (d_idx).vals.at (i);
      }
      mean = sum / points->pts.size ();
      double variance = (double)(sq_sum - sum * sum / points->pts.size ()) / (points->pts.size () - 1);
      stddev = sqrt (variance);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /** \brief Compute both the mean and the standard deviation of a channel/dimension of the cloud using indices
      * \param points a pointer to the point cloud message
      * \param indices a pointer to the point cloud indices to use
      * \param d_idx the channel index
      * \param mean the resultant mean of the distribution
      * \param stddev the resultant standard deviation of the distribution
      */
    void
      getChannelMeanStd (std_msgs::PointCloud *points, std::vector<int> *indices, int d_idx, double &mean, double &stddev)
    {
      double sum = 0, sq_sum = 0;

      for (unsigned int i = 0; i < indices->size (); i++)
      {
        sum += points->chan.at (d_idx).vals.at (indices->at (i));
        sq_sum += points->chan.at (d_idx).vals.at (indices->at (i)) * points->chan.at (d_idx).vals.at (indices->at (i));
      }
      mean = sum / indices->size ();
      double variance = (double)(sq_sum - sum * sum / indices->size ()) / (indices->size () - 1);
      stddev = sqrt (variance);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /** \brief Determines a set of point indices outside of a \mu +/- \sigma * \std distribution interval, in a given
      * channel space, where \mu and \std are the mean and the standard deviation of the channel distribution.
      * \param points a pointer to the point cloud message
      * \param indices a pointer to the point cloud indices to use
      * \param d_idx the channel index
      * \param mean the mean of the distribution
      * \param stddev the standard deviation of the distribution
      * \param alpha a multiplication factor for stddev
      * \param inliers the resultant point indices
      */
    void
      selectPointsOutsideDistribution (std_msgs::PointCloud *points, std::vector<int> *indices, int d_idx,
                                       double mean, double stddev, double alpha, std::vector<int> &inliers)
    {
      inliers.resize (indices->size ());
      int nr_i = 0;
      for (unsigned int i = 0; i < indices->size (); i++)
      {
        if ( (points->chan.at (d_idx).vals.at (indices->at (i)) > (mean + alpha * stddev)) ||
             (points->chan.at (d_idx).vals.at (indices->at (i)) < (mean - alpha * stddev))
           )
        {
          inliers[nr_i] = indices->at (i);
          nr_i++;
        }
      }
      inliers.resize (nr_i);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /** \brief Determines a set of point indices inside of a \mu +/- \sigma * \std distribution interval, in a given
      * channel space, where \mu and \std are the mean and the standard deviation of the channel distribution.
      * \param points a pointer to the point cloud message
      * \param indices a pointer to the point cloud indices to use
      * \param d_idx the channel index
      * \param mean the mean of the distribution
      * \param stddev the standard deviation of the distribution
      * \param alpha a multiplication factor for stddev
      * \param inliers the resultant point indices
      */
    void
      selectPointsInsideDistribution (std_msgs::PointCloud *points, std::vector<int> *indices, int d_idx,
                                      double mean, double stddev, double alpha, std::vector<int> &inliers)
    {
      inliers.resize (indices->size ());
      int nr_i = 0;
      for (unsigned int i = 0; i < indices->size (); i++)
      {
        if ( (points->chan.at (d_idx).vals.at (indices->at (i)) < (mean + alpha * stddev)) &&
             (points->chan.at (d_idx).vals.at (indices->at (i)) > (mean - alpha * stddev))
           )
        {
          inliers[nr_i] = indices->at (i);
          nr_i++;
        }
      }
      inliers.resize (nr_i);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /** \brief Compute both the trimean (a statistical resistent L-Estimate) of an array of values
      * \note The trimean is computed by adding the 25th percentile plus twice the 50th percentile plus the 75th
      * percentile and dividing by four.
      * \param values the array of values
      * \param trimean the resultant trimean of the distribution
      */
    void
      getTrimean (std::vector<int> values, double &trimean)
    {
      nth_element (values.begin (), values.begin () + (int)(values.size () * 0.25), values.end ());
      int p_25 = *(values.begin () + (int)(values.size () * 0.25));

      nth_element (values.begin (), values.begin () + (int)(values.size () * 0.5), values.end ());
      int p_50 = *(values.begin () + (int)(values.size () * 0.5));

      nth_element (values.begin (), values.begin () + (int)(values.size () * 0.75), values.end ());
      int p_75 = *(values.begin () + (int)(values.size () * 0.75));

      trimean = (p_25 + 2.0 * p_50 + p_75) / 4.0;
    }

  }
}
