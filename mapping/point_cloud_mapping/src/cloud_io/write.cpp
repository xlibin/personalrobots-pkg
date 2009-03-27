/*
 * Copyright (c) 2008-2009 Radu Bogdan Rusu <rusu -=- cs.tum.edu>
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

#include <point_cloud_mapping/cloud_io.h>

namespace cloud_io
{
  ////////////////////////////////////////////////////////////////////////////////
  // Create the rest of the needed string values from the given header
  std::string
    addCurrentHeader (const robot_msgs::PointCloud &points, bool binary_type)
  {
    // Must have a non-null number of points
    if (points.pts.size () == 0)
      return ("");

    std::string result = "COLUMNS ";
    result += getAvailableDimensions (points);

    std::ostringstream oss;
    oss << result << "\nPOINTS " << points.pts.size () << "\n";

    oss << "DATA ";
    if (binary_type)
      oss << "binary\n";
    else
      oss << "ascii\n";

    result = oss.str ();
    return (result);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief Save point cloud data to a PCD file containing n-D points, in ASCII format
    * \param file_name the output file name
    * \param points the point cloud data message
    * \param precision the specified output numeric stream precision
    */
  int
    savePCDFileASCII (const char* file_name, const robot_msgs::PointCloud &points, int precision)
  {
    std::ofstream fs;
    fs.precision (precision);
    fs.open (file_name);      // Open file

    // Write the header information
    fs << createNewHeader ();

    std::string header_values = addCurrentHeader (points, false);
    if (header_values == "")
    {
      ROS_ERROR ("Error assembling header! Possible reasons: no COLUMNS indices, or no POINTS.");
      return (-1);
    }
    fs << header_values;

    // Iterate through the points
    for (unsigned int i = 0; i < points.pts.size (); i++)
    {
      fs << points.pts[i].x << " " << points.pts[i].y << " " << points.pts[i].z;
      for (unsigned int d = 0; d < points.chan.size (); d++)
        fs << " " << points.chan[d].vals[i];
      fs << std::endl;
    }
    fs.close ();              // Close file
    return (0);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief Save point cloud data to a PCD file containing n-D points, in BINARY format
    * \param file_name the output file name
    * \param points the point cloud data message
    */
  int
    savePCDFileBinary (const char* file_name, const robot_msgs::PointCloud &points)
  {
    int data_idx = 0;
    std::ofstream fs;
    // Open file
    fs.open (file_name);

    // Write the header information
    fs << createNewHeader ();

    std::string header_values = addCurrentHeader (points, true);
    if (header_values == "")
    {
      ROS_ERROR ("Error assembling header! Possible reasons: no COLUMNS indices, or no POINTS.");
      return (-1);
    }
    fs << header_values;
    data_idx = fs.tellp ();
    // Close file
    fs.close ();

    if (data_idx > getpagesize ())
    {
      ROS_WARN ("Header size (%d) is bigger than page size (%d)! Reduce the number of channels or save in ASCII format. Removing extra comments...\n", data_idx, getpagesize ());

      fs.open (file_name);
      fs << createNewHeaderShort ();
      std::string header_values = addCurrentHeader (points, true);
      if (header_values == "")
      {
        ROS_ERROR ("Error assembling header! Possible reasons: no COLUMNS indices, or no POINTS.");
        return (-1);
      }
      fs << header_values;
      data_idx = fs.tellp ();
      // Close file
      fs.close ();
      if (data_idx > getpagesize ())
      {
        ROS_ERROR ("Header size (%d) is *still* bigger than page size (%d) even after I removed the extra comments! Please consider saving the file into ASCII format.", data_idx, getpagesize ());
        return (-1);
      }
    }

    // Compute how much a point (with it's N-dimensions) occupies in terms of bytes
    int point_size = sizeof (float) * (3 + points.chan.size ());

    // Open for writing
    int fd = open (file_name, O_RDWR);
    if (fd == -1)
    {
      ROS_ERROR ("[savePCDFileBinary] Error during open ()!");
      return (-1);
    }

    // Stretch the file size to the size of the data
    int result = lseek (fd, getpagesize () + (points.pts.size () * point_size) - 1, SEEK_SET);
    if (result == -1)
    {
      close (fd);
      ROS_ERROR ("[savePCDFileBinary] Error during lseek ()!");
      return (-1);
    }
    // Write a bogus entry so that the new file size comes in effect
    result = write (fd, "", 1);
    if (result != 1)
    {
      close (fd);
      ROS_ERROR ("[savePCDFileBinary] Error during write ()!");
      return (-1);
    }

    // Prepare the map
    char *map = (char*)mmap (0, points.pts.size () * point_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, getpagesize ());
    if (map == MAP_FAILED)
    {
      close (fd);
      ROS_ERROR ("[savePCDFileBinary] Error during mmap ()!");
      return (-1);
    }

    // Write the data
    for (unsigned int i = 0; i < points.pts.size (); i++)
    {
      int idx_j = 0;
      memcpy ((char*)&map[i * point_size + idx_j], &points.pts[i].x, sizeof (float));
      idx_j += sizeof (float);
      memcpy ((char*)&map[i * point_size + idx_j], &points.pts[i].y, sizeof (float));
      idx_j += sizeof (float);
      memcpy ((char*)&map[i * point_size + idx_j], &points.pts[i].z, sizeof (float));
      idx_j += sizeof (float);

      for (unsigned int j = 0; j < points.chan.size (); j++)
      {
        memcpy ((char*)&map[i * point_size + idx_j], &points.chan[j].vals[i], sizeof (float));
        idx_j += sizeof (float);
      }
    }

    // Unmap the pages of memory
    if (munmap (map, (points.pts.size () * point_size)) == -1)
    {
      close (fd);
      ROS_ERROR ("[savePCDFileBinary] Error during munmap ()!");
      return (-1);
    }

    // Close file
    close (fd);
    return (0);
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  /** \brief Save point cloud data to a PCD file containing n-D points
    * \param file_name the output file name
    * \param points the point cloud data message
    * \param binary_mode true for binary mode, false for ascii
    */
  int
    savePCDFile (const char* file_name, const robot_msgs::PointCloud &points, bool binary_mode)
  {
    if (binary_mode)
      return (savePCDFileBinary (file_name, points));
    else
      return (savePCDFileASCII (file_name, points, 5));
  }

}
