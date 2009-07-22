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

#include <Eigen/Array>
#include <Eigen/SVD>
#include <pr2_msgs/Odometry.h>
#include <realtime_tools/realtime_publisher.h>
#include <tf/tfMessage.h>
#include <pr2_mechanism_controllers/base_kinematics.h>
#include <deprecated_msgs/RobotBase2DOdom.h>
#include <angles/angles.h>

typedef Eigen::Matrix<float, 3, 1> OdomMatrix3x1;
typedef Eigen::Matrix<float, 16, 1> OdomMatrix16x1;
typedef Eigen::Matrix<float, 16, 3> OdomMatrix16x3;
typedef Eigen::Matrix<float, 16, 16> OdomMatrix16x16;

namespace controller
{
  /*! \class
  \brief This class inherits from Controller and computes the base odometry
  */
  class Pr2Odometry : public Controller
  {
    public:

    /*!
    * \brief Constructor for the odometry
    */
    Pr2Odometry();

    /*!
    * \brief Destructor for the odometry
    */
    ~Pr2Odometry();

    /*!
    * \brief Loads Odometry's information from the xml description file and param server
    * @param robot_state The robot's current state
    * @param config Tiny xml element pointing to this controller
    * @return Successful init
    */
    bool initXml(mechanism::RobotState *robot_state, TiXmlElement *config);

    /*!
    * \brief Initializes and loads odometry information from the param server
    * @param robot_state The robot's current state
    * @param config Tiny xml element pointing to this controller
    * @return Successful init
    */
    bool init(mechanism::RobotState *robot_state, const ros::NodeHandle &node);

    /*
    * \brief  The starting method is called by the realtime thread just before
    * the first call to update.  Overrides Controller.staring().
    * @return Successful start
    */
    bool starting();

    /*!
    * \brief (a) Updates positions of the caster and wheels.
    * Called every timestep in realtime
    */
    void update();

    /*!
    * \brief Publishes the currently computed odometry information
    */
    void publish();


    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    private:

    ros::NodeHandle node_;

    /*!
    * \brief class where the robot's information is computed and stored
    */
    BaseKinematics base_kin_;

    /*!
    * \brief Finds and stores the latest odometry information
    */
    void updateOdometry();

    /*!
    * \brief Builds the odometry message and prepares it for sending
    * @param msg The pr2_msgs::Odometry into which the odometry values are placed
    */
    void getOdometryMessage(pr2_msgs::Odometry &msg);
    
    /*!
    * \brief Takes the current odometery information and stores it into the six double parameters
    * @param x X component of the current odom position
    * @param y Y component of the current odom position
    * @param yaw Yaw (theta) component of the current odom position
    * @param vx X component of the current odom velocity
    * @param vy Y component of the current odom velocity
    * @param vw Angular velocity (omega) component of the current odom velocity
    */
    void getOdometry(double &x, double &y, double &yaw, double &vx, double &vy, double &vw);

    /*!
    * \brief Takes the current odometry information and stores it into the Point and PoseDot parameters
    * @param odom Point into which the odometry position is placed (z is theta)
    * @param odom_vel into which the odometry velocity is placed
    */
    void getOdometry(robot_msgs::Point &odom, robot_msgs::PoseDot &odom_vel);

    /*!
    * \brief Computes the base velocity from the caster positions and wheel speeds
    */
    void computeBaseVelocity();

    /*!
    * \brief Computes the wheel's speed adjusted for the attached caster's rotational velocity
    * @param index The index of the wheel
    */
    double getCorrectedWheelSpeed(int index);

    /*!
    * \brief Function used to compute the most likely solution to the odometry using iterative least squares
    */
    Eigen::MatrixXf iterativeLeastSquares(Eigen::MatrixXf lhs, Eigen::MatrixXf rhs, std::string weight_type, int max_iter);
    
    /*!
    * \brief Finds the weight matrix from the iterative least squares residuals
    */
    Eigen::MatrixXf findWeightMatrix(Eigen::MatrixXf residual, std::string weight_type);

    /*!
    * \brief Total distance traveled by the base as computed by the odometer
    */
    double odometer_distance_;

    /*!
    * \brief Total angular distance traveled by the base as computed by the odometer
    */
    double odometer_angle_;

    /*!
    * \brief Stores the last update time and the current update time
    */
    double last_time_,current_time_;

    /*!
    * \brief Matricies used in the computation of the iterative least squares and related functions
    */
    Eigen::MatrixXf cbv_rhs_, fit_rhs_, fit_residual_, odometry_residual_, cbv_lhs_, fit_lhs_, cbv_soln_,fit_soln_,  weight_matrix_;

    /*!
    * \brief Point that stores the current translational position (x,y) and angular position (z)
    */
    robot_msgs::Point odom_;

    /*!
    * \brief PoseDot that remembers the current translational velocities (vel.vx, vel.vy) and angular position (ang_vel.vz)
    */
    robot_msgs::PoseDot odom_vel_;

    /*!
    * \brief The type of weighting used in findWeightMatrix
    */
    std::string ils_weight_type_;

    /*!
    * \brief Number of iterations used during iterative least squares
    */
    int ils_max_iterations_;

    /*!
    * \brief Maximum residual from the iteritive least squares
    */
    double odometry_residual_max_;

    /*!
    * \brief The topic name of the published odometry
    */
    std::string odom_frame_;

    /*!
    * \brief The last time the odometry information was published
    */
    double last_publish_time_;

    /*!
    * \brief The time that the odometry is expected to be published next
    */
    double expected_publish_time_;

    /*!
    * \brief The RealtimePublisher that does the realtime publishing of the odometry
    */
    realtime_tools::RealtimePublisher <pr2_msgs::Odometry>* odometry_publisher_ ;  

    /*!
    * \brief The RealtimePublisher that does the realtime publishing of the old style odometry
    */
    realtime_tools::RealtimePublisher <deprecated_msgs::RobotBase2DOdom>* old_odometry_publisher_ ;  

    /*!
    * \brief Publishes the transform between the odometry frame and the base frame
    */
    realtime_tools::RealtimePublisher <tf::tfMessage>* transform_publisher_ ;  

    /*!
    * \brief Creates an old odometry message from the new odometery message
    * @param msg The old message into which the odometry will be placed
    */
    void getOldOdometryMessage(deprecated_msgs::RobotBase2DOdom &msg);

  };
}
