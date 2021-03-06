cmake_minimum_required (VERSION 2.4.6)

### If you're trying to debug include directives or so, this here
### might help:
#set (CMAKE_VERBOSE_MAKEFILE ON)

set (ROS_BUILD_TYPE Release)
#set (ROS_BUILD_TYPE Debug)
include ($ENV{ROS_ROOT}/core/rosbuild/rosbuild.cmake)
add_definitions (-Wall)
rosbuild_init ()

##rosbuild_genmsg ()
rosbuild_gensrv ()

list (APPEND MPGLUE_SRCS
      src/setup.cpp
      src/costmap.cpp
      src/costmapper.cpp
      src/planner.cpp
      src/sbpl_planner.cpp
      src/navfn_planner.cpp
      src/footprint.cpp
      src/sbpl_environment.cpp
      src/plan.cpp)

set (ROS_ESTAR_DIR $ENV{ROS_ESTAR_DIR})
if (ROS_ESTAR_DIR)
  message (STATUS "ROS_ESTAR_DIR is ${ROS_ESTAR_DIR}")
  set (CMAKE_REQUIRED_INCLUDES ${ROS_ESTAR_DIR}/include)
  include_directories (${ROS_ESTAR_DIR}/include)
  link_directories (${ROS_ESTAR_DIR}/lib)
else (ROS_ESTAR_DIR)
  message (STATUS "ROS_ESTAR_DIR is not set")
endif (ROS_ESTAR_DIR)

include (CheckIncludeFileCXX)
check_include_file_cxx (estar/Facade.hpp MPGLUE_HAVE_ESTAR)
if (MPGLUE_HAVE_ESTAR)
  message (STATUS "found E*, adding src/estar_planner.cpp to build")
  add_definitions (-DMPGLUE_HAVE_ESTAR)
  list (APPEND MPGLUE_SRCS src/estar_planner.cpp)
  list (APPEND MPGLUE_LIBS estar)
else (MPGLUE_HAVE_ESTAR)
  message (STATUS "E* not found (optional)")
endif (MPGLUE_HAVE_ESTAR)

rosbuild_add_library (mpglue ${MPGLUE_SRCS})
target_link_libraries (mpglue ${MPGLUE_LIBS})

rosbuild_add_gtest (test/mpglue_test test/mpglue_test.cpp)
target_link_libraries (test/mpglue_test mpglue)

rosbuild_add_executable (bin/costmap_planner_node src/costmap_planner_node.cpp)
target_link_libraries (bin/costmap_planner_node mpglue)

rosbuild_add_executable(bin/test_costmap_planner_node src/test_costmap_planner_node.cpp)
rosbuild_add_gtest_build_flags(bin/test_costmap_planner_node)
target_link_libraries (bin/test_costmap_planner_node mpglue)
rosbuild_add_rostest(test/test_costmap_planner_node.launch)
