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

#include <pr2_controllers/base_controller.h>
#include <math_utils/angles.h>


using namespace std;
using namespace controller;
using namespace libTF;
using namespace NEWMAT;

ROS_REGISTER_CONTROLLER(BaseController)


BaseController::BaseController() : num_wheels_(0), num_casters_(0)
{
}

BaseController::~BaseController()
{
}

// Set the joint position command
void BaseController::setCommand(libTF::Pose3D::Vector cmd_vel)
{
  pthread_mutex_lock(&base_controller_lock_);
  cmd_vel_t_.x = cmd_vel.x;
  cmd_vel_t_.y = cmd_vel.y;
  cmd_vel_t_.z = cmd_vel.z;
  pthread_mutex_unlock(&base_controller_lock_);
}

libTF::Pose3D::Vector BaseController::getCommand()// Return the current position command
{
  libTF::Pose3D::Vector cmd_vel;
  pthread_mutex_lock(&base_controller_lock_);
  cmd_vel.x = cmd_vel_t_.x;
  cmd_vel.y = cmd_vel_t_.y;
  cmd_vel.z = cmd_vel_t_.z;
  pthread_mutex_unlock(&base_controller_lock_);
  return cmd_vel;
}

void BaseController::init(std::vector<JointControlParam> jcp, mechanism::Robot *robot)
{
  std::vector<JointControlParam>::iterator jcp_iter;
  robot_desc::URDF::Link *link;
  std::string joint_name;

  for(jcp_iter = jcp.begin(); jcp_iter != jcp.end(); jcp_iter++)
  {
    joint_name = jcp_iter->joint_name;
    link = urdf_model_.getJointLink(joint_name);

    BaseParam base_object;
    base_object.pos_.x = link->xyz[0];
    base_object.pos_.y = link->xyz[1];
    base_object.pos_.z = link->xyz[2];
    base_object.name_ = joint_name;
    base_object.parent_ = NULL;
    base_object.joint_ = robot->getJoint(joint_name);
    base_object.controller_.init(jcp_iter->p_gain,jcp_iter->i_gain,jcp_iter->d_gain,jcp_iter->windup,0.0,jcp_iter->joint_name,robot);

    if(joint_name.find("caster") != string::npos)
    {
      base_object.local_id_ = num_casters_;
      base_casters_.push_back(base_object);      
      num_casters_++;
    }
    if(joint_name.find("wheel") != string::npos)
    {
      base_object.local_id_ = num_wheels_;
      base_wheels_.push_back(base_object);
      num_wheels_++;
    }
  }

  for(int i =0; i < num_wheels_; i++)
  {
    link = urdf_model_.getJointLink(base_wheels_[i].name_);
    std::string parent_name = link->parent->joint->name;
    for(int j =0; j < num_casters_; j++)
    {
      if(parent_name == base_casters_[j].name_)
      {
        base_wheels_[i].parent_ = &base_casters_[j];
        break;
      }
    }
  }

  robot_ = robot;
}

void BaseController::initXml(mechanism::Robot *robot, TiXmlElement *config)
{
  TiXmlElement *elt = config->FirstChildElement("controller");
  std::vector<JointControlParam> jcp_vec;
  JointControlParam jcp;
  while (elt){
    TiXmlElement *jnt = elt->FirstChildElement("joint");

    // TODO: error check if xml attributes/elements are missing
    jcp.p_gain = atof(elt->FirstChildElement("controller_defaults")->Attribute("p"));
    jcp.i_gain = atof(elt->FirstChildElement("controller_defaults")->Attribute("i"));
    jcp.d_gain = atof(elt->FirstChildElement("controller_defaults")->Attribute("d"));
    jcp.windup = atof(elt->FirstChildElement("controller_defaults")->Attribute("iClamp"));
    jcp.control_type = (std::string) elt->Attribute("type");
    jcp.joint_name = jnt->Attribute("name");
    jcp_vec.push_back(jcp);

    elt = config->NextSiblingElement("controller");
  }
  elt = config->FirstChildElement("map");
  while(elt)
  {
    if(elt->Attribute("name") == "velocity_control")
    {
      TiXmlElement *elt_key = elt->FirstChildElement("elem");
      while(elt_key)
      {
        if(elt_key->Attribute("key") == "kp_speed")
        {
          kp_speed_ = atof(elt_key->GetText());
          break;
        }
        elt_key = elt->NextSiblingElement("elem");
      }
    }
    elt = config->NextSiblingElement("map");
  }
  init(jcp_vec,robot);
}

void BaseController::getJointValues()
{
  for(int i=0; i < num_casters_; i++)
    steer_angle_actual_[i] = base_casters_[i].joint_->position_;

  for(int i=0; i < num_wheels_; i++)
    wheel_speed_actual_[i] = base_wheels_[i].controller_.getMeasuredVelocity();
}

void BaseController::computeWheelPositions()
{
  libTF::Pose3D::Vector res1;
  double steer_angle;
  for(int i=0; i < num_wheels_; i++)
  {
    steer_angle = base_wheels_[i].parent_->joint_->position_;
    res1 = rotate2D(base_wheels_[i].pos_,steer_angle);
    res1 += base_casters_[i].pos_;
    base_wheels_position_[i] = res1;
  }
}


void BaseController::update()
{
  double current_time = robot_->hw_->current_time_;

  if(pthread_mutex_trylock(&base_controller_lock_)==0)
  {
    cmd_vel_.x = cmd_vel_t_.x;
    cmd_vel_.y = cmd_vel_t_.y;
    cmd_vel_.z = cmd_vel_t_.z;
    pthread_mutex_unlock(&base_controller_lock_);
  }

  getJointValues();

  computeWheelPositions();

  computeAndSetCasterSteer();

  computeAndSetWheelSpeeds();

  computeOdometry(current_time);

  updateJointControllers();

  last_time_ = current_time;
}

double ModNPiBy2(double angle)
{
  if (angle < -M_PI/2) 
    angle += M_PI;
  if(angle > M_PI/2)
    angle -= M_PI;
  return angle;
}

void BaseController::computeAndSetCasterSteer()
{
  libTF::Pose3D::Vector result;
  double steer_angle_desired;
  double kp_local = 10;
  for(int i=0; i < num_casters_; i++)
  {
    result = computePointVelocity2D(base_casters_[i].pos_, cmd_vel_);
    steer_angle_desired = atan2(result.y,result.x);
    steer_angle_desired = ModNPiBy2(steer_angle_desired);//Clean steer Angle    
    steer_velocity_desired_[i] = kp_local*steer_angle_desired;
    base_casters_[i].controller_.setCommand(steer_velocity_desired_[i]);
  } 
}

void BaseController::computeAndSetWheelSpeeds()
{
  libTF::Pose3D::Vector wheel_point_velocity;
  libTF::Pose3D::Vector wheel_point_velocity_projected;
  libTF::Pose3D::Vector wheel_caster_steer_component;
  libTF::Pose3D::Vector caster_2d_velocity;

  caster_2d_velocity.x = 0;
  caster_2d_velocity.y = 0;
  caster_2d_velocity.z = 0;

  double wheel_speed_cmd = 0;
  double steer_angle_actual = 0;
  for(int i=0; i < (int) num_wheels_; i++)
  {
    caster_2d_velocity.z = steer_velocity_desired_[base_wheels_[i].parent_->local_id_];
    steer_angle_actual = base_wheels_[i].parent_->joint_->position_;
    wheel_point_velocity = computePointVelocity2D(base_wheels_position_[i],cmd_vel_);
    wheel_caster_steer_component = computePointVelocity2D(base_wheels_[i].pos_,caster_2d_velocity);
    wheel_point_velocity_projected = rotate2D(wheel_point_velocity,-steer_angle_actual);
    wheel_speed_cmd = (wheel_point_velocity_projected.x + wheel_caster_steer_component.x)/wheel_radius_;     
    base_wheels_[i].controller_.setCommand(wheel_speed_cmd);
  }
}

void BaseController::updateJointControllers()
{
  for(int i=0; i < num_wheels_; i++)
    base_wheels_[i].controller_.update();
  for(int i=0; i < num_casters_; i++)
    base_casters_[i].controller_.update();
}

ROS_REGISTER_CONTROLLER(BaseControllerNode)
BaseControllerNode::BaseControllerNode() 
{
  c_ = new BaseController();
}

BaseControllerNode::~BaseControllerNode()
{
  delete c_;
}

void BaseControllerNode::update()
{
  c_->update();
}

bool BaseControllerNode::setCommand(
  pr2_controllers::SetBaseCommand::request &req,
  pr2_controllers::SetBaseCommand::response &resp)
{
   libTF::Pose3D::Vector command;
   command.x = req.x_vel;
   command.y = req.y_vel;
   command.z = req.theta_vel;
   c_->setCommand(command);
   command = c_->getCommand();
   resp.x_vel = command.x;
   resp.y_vel = command.y;
   resp.theta_vel = command.z;

  return true;
}

bool BaseControllerNode::getCommand(
  pr2_controllers::GetBaseCommand::request &req,
  pr2_controllers::GetBaseCommand::response &resp)
{
   libTF::Pose3D::Vector command;
   command = c_->getCommand();
   resp.x_vel = command.x;
   resp.y_vel = command.y;
   resp.theta_vel = command.z;

  return true;
}

void BaseControllerNode::initXml(mechanism::Robot *robot, TiXmlElement *config)
{
  ros::node *node = ros::node::instance();
  string prefix = config->Attribute("name");

  std::string xml_content;
  node->get_param("robotdesc/pr2",xml_content);

  c_->initXml(robot, config);
  node->advertise_service(prefix + "/set_command", &BaseControllerNode::setCommand, this);
  node->advertise_service(prefix + "/get_command", &BaseControllerNode::getCommand, this);

  if(!c_->urdf_model_.loadString(xml_content.c_str()))
     return;

}

Pose3D::Vector BaseController::computePointVelocity2D(const Pose3D::Vector& pos, const Pose3D::Vector& vel)
{
  Pose3D::Vector result;
  
  result.x = vel.x - pos.y * vel.z;
  result.y = vel.y + pos.x * vel.z;
  result.z = 0;

  return result;
}

Pose3D::Vector BaseController::rotate2D(const Pose3D::Vector& pos, double theta)
{
  Pose3D::Vector result;
  
  result.x = cos(theta)*pos.x - sin(theta)*pos.y;
  result.y = sin(theta)*pos.x + cos(theta)*pos.y;
  result.z = 0;

  return result;
}

void BaseController::computeOdometry(double time) 
{
   double dt = time-last_time_;
   libTF::Pose3D::Vector base_odom_delta = rotate2D(base_odom_velocity_*dt,base_odom_position_.z);
   base_odom_delta.z = base_odom_velocity_.z * dt;
   base_odom_position_ += base_odom_delta;

   odomMsg.pos.x  = base_odom_position_.x;
   odomMsg.pos.y  = base_odom_position_.y;
   odomMsg.pos.th = base_odom_position_.z;

   odomMsg.vel.x  = base_odom_velocity_.x;
   odomMsg.vel.y  = base_odom_velocity_.y;
   odomMsg.vel.th = base_odom_velocity_.z;
}

void BaseController::computeBaseVelocity()
{
  Matrix A(2*num_wheels_,1);
  Matrix C(2*num_wheels_,3);
  Matrix D(3,1);
  double steer_angle;

  for(int i = 0; i < num_wheels_; i++) {
    steer_angle = base_wheels_[i].parent_->joint_->position_;
    A.element(i*2,0)   = cos(steer_angle)*wheel_radius_*(wheel_speed_actual_[i]);
    A.element(i*2+1,0) = sin(steer_angle)*wheel_radius_*(wheel_speed_actual_[i]);
  }

  for(int i = 0; i < num_wheels_; i++) {
    C.element(i*2, 0)   = 1;
    C.element(i*2, 1)   = 0;
    C.element(i*2, 2)   = -base_wheels_position_[i].y;
    C.element(i*2+1, 0) = 0;
    C.element(i*2+1, 1) = 1;
    C.element(i*2+1, 2) =  base_wheels_position_[i].x;
  }
  D = pseudoInverse(C)*A; 
  base_odom_velocity_.x = (double)D.element(0,0);
  base_odom_velocity_.y = (double)D.element(1,0);
  base_odom_velocity_.z = (double)D.element(2,0);
}

Matrix BaseController::pseudoInverse(const Matrix M)
{
  Matrix result;
  //int rows = this->rows();
  //int cols = this->columns();
  // calculate SVD decomposition
  Matrix U,V;
  DiagonalMatrix D;
  NEWMAT::SVD(M,D,U,V, true, true);
  Matrix Dinv = D.i();
  result = V * Dinv * U.t();
  return result;
}



// void BaseController::setGeomParams()
// {
//   char *c_filename = getenv("ROS_PACKAGE_PATH");
//   std::stringstream filename;
//   filename << c_filename << "/robot_descriptions/wg_robot_description/pr2/pr2.xml" ;
//   robot_desc::URDF model;
//   if(!model.loadFile(filename.str().c_str()))
//      return;

//   robot_desc::URDF::Group *base_group = model.getGroup("base_control");
//   robot_desc::URDF::Link *base_link;
//   robot_desc::URDF::Link *caster_link;
//   robot_desc::URDF::Link *wheel_link;

// //  double base_caster_x_offset(0), base_caster_y_offset(0), wheel_base_(0);

//   if((int) base_group->linkRoots.size() != 1)
//   {
//     fprintf(stderr,"base_control.cpp::Too many roots in base!\n");
//   }
//   base_link = base_group->linkRoots[0];
//   caster_link = *(base_link->children.begin());
//   wheel_link = *(caster_link->children.begin());

//   base_caster_x_offset_ = fabs(caster_link->xyz[0]);
//   base_caster_y_offset_ = fabs(caster_link->xyz[1]);
//   wheel_base_ =2*sqrt(wheel_link->xyz[0]*wheel_link->xyz[0]+wheel_link->xyz[1]*wheel_link->xyz[1]);

//   robot_desc::URDF::Link::Geometry::Cylinder *wheel_geom = dynamic_cast<robot_desc::URDF::Link::Geometry::Cylinder*> (wheel_link->collision->geometry->shape);
//   wheel_radius_ = wheel_geom->radius;

//   BaseCasterGeomParam caster;
//   libTF::Pose3D::Vector wheel_l;
//   libTF::Pose3D::Vector wheel_r;

//   wheel_l.x = 0;
//   wheel_l.y = wheel_base_/2.0;
//   wheel_l.z = 0;

//   wheel_r.x = 0;
//   wheel_r.y = -wheel_base_/2.0;
//   wheel_r.z = 0;

//   caster.wheel_pos.push_back(wheel_l);
//   caster.wheel_pos.push_back(wheel_r);

// // FRONT LEFT
//   caster.pos.x = base_caster_x_offset_;
//   caster.pos.y = base_caster_y_offset_;
//   caster.pos.z = 0;
//   base_casters_.push_back(caster);

// // FRONT RIGHT
//   caster.pos.x = base_caster_x_offset_;
//   caster.pos.y = -base_caster_y_offset_;
//   caster.pos.z = 0;
//   base_casters_.push_back(caster);

// // REAR LEFT
//   caster.pos.x = -base_caster_x_offset_;
//   caster.pos.y = base_caster_y_offset_;
//   caster.pos.z = 0;
//   base_casters_.push_back(caster);

// // REAR RIGHT
//   caster.pos.x = -base_caster_x_offset_;
//   caster.pos.y = -base_caster_y_offset_;
//   caster.pos.z = 0;
//   base_casters_.push_back(caster);
// }
// void BaseController::computeBaseVelocity()
// {

//   Matrix A(2*NUM_WHEELS,1);
//   //Matrix B(NUM_WHEELS,1);
//   Matrix C(2*NUM_WHEELS,3);
//   Matrix D(3,1);
  
//   for(int i = 0; i < NUM_CASTERS; i++) {
//     A.element(i*4,0) = cos(robot->joint[i*3].position) *WHEEL_RADIUS*((double)-1)*robot->joint[i*3+1].velocity;
//     A.element(i*4+1,0) = sin(robot->joint[i*3].position) *WHEEL_RADIUS*((double)-1)*robot->joint[i*3+1].velocity;
//     A.element(i*4+2,0) = cos(robot->joint[i*3].position) *WHEEL_RADIUS*robot->joint[i*3+2].velocity;
//     A.element(i*4+3,0) = sin(robot->joint[i*3].position)* WHEEL_RADIUS*robot->joint[i*3+2].velocity;      
//   }

//   /*
//     for(int i = 0; i < (NUM_WHEELS + NUM_CASTERS); i++) {
//     printf("i: %i pos : %03f vel: %03f\n", i,robot->joint[i].position, robot->joint[i].velocity); 
//     }
//   */
//   for(int i = 0; i < NUM_CASTERS; i++) {
//     C.element(i*4, 0) = 1;
//     C.element(i*4, 1) = 0;
//     C.element(i*4, 2) = -(Rot2D(CASTER_DRIVE_OFFSET[i*2].x,CASTER_DRIVE_OFFSET[i*2].y,robot->joint[i*3].position).y + BASE_CASTER_OFFSET[i].y);
//     C.element(i*4+1, 0) = 0;
//     C.element(i*4+1, 1) = 1;
//     C.element(i*4+1, 2) =  Rot2D(CASTER_DRIVE_OFFSET[i*2].x,CASTER_DRIVE_OFFSET[i*2].y,robot->joint[i*3].position).x + BASE_CASTER_OFFSET[i].x;
//     C.element(i*4+2, 0) = 1;
//     C.element(i*4+2, 1) = 0;
//     C.element(i*4+2, 2) =  -(Rot2D(CASTER_DRIVE_OFFSET[i*2+1].x,CASTER_DRIVE_OFFSET[i*2+1].y,robot->joint[i*3].position).y + BASE_CASTER_OFFSET[i].y);
//     C.element(i*4+3, 0) = 0;
//     C.element(i*4+3, 1) = 1;
//     C.element(i*4+3, 2) =  Rot2D(CASTER_DRIVE_OFFSET[i*2+1].x,CASTER_DRIVE_OFFSET[i*2+1].y,robot->joint[i*3].position).x + BASE_CASTER_OFFSET[i].x;
//   }

//   D = pseudoInverse(C)*A; 
//   /*
//     aTest = C*commandTest;
//     cout << "A:" << endl;
//     cout << A;
//     cout << "C :" << endl;
//     cout << C<< endl;
//     cout << "commandTest: "<< endl;
//     cout << commandTest << endl;
//     cout << "aTest: "<< endl;
//     cout << aTest << endl;
//     //   
//     //
//   */
//   base_odom_vx_ = (double)D.element(0,0);
//   base_odom_vy_ = (double)D.element(1,0);
//   base_odom_vw_ = (double)D.element(2,0);
//   //cout << "D :" << endl;  
//   //cout << D << endl;
// }

// Matrix BaseController::pseudoInverse(const Matrix M)
// {
//   Matrix result;
//   //int rows = this->rows();
//   //int cols = this->columns();
//   // calculate SVD decomposition
//   Matrix U,V;
//   DiagonalMatrix D;
//   SVD(M,D,U,V, true, true);
//   Matrix Dinv = D.i();
//   result = V * Dinv * U.t();
//   return result;
// }


