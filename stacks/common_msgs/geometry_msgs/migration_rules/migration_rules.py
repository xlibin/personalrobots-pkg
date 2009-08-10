class update_robot_msgs_Vector3_4a842b65f413084dc2b10fb484ea7f17(MessageUpdateRule):
	old_type = "robot_msgs/Vector3"
	old_full_text = """
float64 x
float64 y
float64 z
"""

	new_type = "geometry_msgs/Vector3"
	new_full_text = """
float64 x
float64 y
float64 z
"""

	order = 0
	migrated_types = []

	valid = True

	def update(self, old_msg, new_msg):
		new_msg.x = old_msg.x
		new_msg.y = old_msg.y
		new_msg.z = old_msg.z


class update_robot_msgs_Quaternion_a779879fadf0160734f906b8c19c7004(MessageUpdateRule):
	old_type = "robot_msgs/Quaternion"
	old_full_text = """
# xyz - vector rotation axis, w - scalar term (cos(ang/2))
float64 x
float64 y
float64 z
float64 w
"""

	new_type = "geometry_msgs/Quaternion"
	new_full_text = """
# xyz - vector rotation axis, w - scalar term (cos(ang/2))
float64 x
float64 y
float64 z
float64 w
"""

	order = 0
	migrated_types = []

	valid = True

	def update(self, old_msg, new_msg):
		new_msg.x = old_msg.x
		new_msg.y = old_msg.y
		new_msg.z = old_msg.z
		new_msg.w = old_msg.w

class update_robot_msgs_QuaternionStamped_e57f1e547e0e1fd13504588ffc8334e2(MessageUpdateRule):
	old_type = "robot_msgs/QuaternionStamped"
	old_full_text = """
Header header
Quaternion quaternion

================================================================================
MSG: roslib/Header
#Standard metadata for higher-level flow data types
#sequence ID: consecutively increasing ID 
uint32 seq
#Two-integer timestamp that is expressed as:
# * stamp.secs: seconds (stamp_secs) since epoch
# * stamp.nsecs: nanoseconds since stamp_secs
# time-handling sugar is provided by the client library
time stamp
#Frame this data is associated with
# 0: no frame
# 1: global frame
string frame_id

================================================================================
MSG: robot_msgs/Quaternion
# xyz - vector rotation axis, w - scalar term (cos(ang/2))
float64 x
float64 y
float64 z
float64 w
"""

	new_type = "geometry_msgs/QuaternionStamped"
	new_full_text = """
Header header
Quaternion quaternion

================================================================================
MSG: roslib/Header
#Standard metadata for higher-level flow data types
#sequence ID: consecutively increasing ID 
uint32 seq
#Two-integer timestamp that is expressed as:
# * stamp.secs: seconds (stamp_secs) since epoch
# * stamp.nsecs: nanoseconds since stamp_secs
# time-handling sugar is provided by the client library
time stamp
#Frame this data is associated with
# 0: no frame
# 1: global frame
string frame_id

================================================================================
MSG: geometry_msgs/Quaternion
# xyz - vector rotation axis, w - scalar term (cos(ang/2))
float64 x
float64 y
float64 z
float64 w
"""

	order = 0
	migrated_types = [
		("Header","Header"),
		("Quaternion","Quaternion"),]

	valid = True

	def update(self, old_msg, new_msg):
		self.migrate(old_msg.header, new_msg.header)
		self.migrate(old_msg.quaternion, new_msg.quaternion)

class update_robot_msgs_Vector3Stamped_7b324c7325e683bf02a9b14b01090ec7(MessageUpdateRule):
	old_type = "robot_msgs/Vector3Stamped"
	old_full_text = """
Header header
Vector3 vector

================================================================================
MSG: roslib/Header
#Standard metadata for higher-level flow data types
#sequence ID: consecutively increasing ID 
uint32 seq
#Two-integer timestamp that is expressed as:
# * stamp.secs: seconds (stamp_secs) since epoch
# * stamp.nsecs: nanoseconds since stamp_secs
# time-handling sugar is provided by the client library
time stamp
#Frame this data is associated with
# 0: no frame
# 1: global frame
string frame_id

================================================================================
MSG: robot_msgs/Vector3
float64 x
float64 y
float64 z
"""

	new_type = "geometry_msgs/Vector3Stamped"
	new_full_text = """
Header header
Vector3 vector

================================================================================
MSG: roslib/Header
#Standard metadata for higher-level flow data types
#sequence ID: consecutively increasing ID 
uint32 seq
#Two-integer timestamp that is expressed as:
# * stamp.secs: seconds (stamp_secs) since epoch
# * stamp.nsecs: nanoseconds since stamp_secs
# time-handling sugar is provided by the client library
time stamp
#Frame this data is associated with
# 0: no frame
# 1: global frame
string frame_id

================================================================================
MSG: geometry_msgs/Vector3
float64 x
float64 y
float64 z
"""

	order = 0
	migrated_types = [
		("Header","Header"),
		("Vector3","Vector3"),]

	valid = True

	def update(self, old_msg, new_msg):
		self.migrate(old_msg.header, new_msg.header)
		self.migrate(old_msg.vector, new_msg.vector)

class update_robot_msgs_Point_4a842b65f413084dc2b10fb484ea7f17(MessageUpdateRule):
	old_type = "robot_msgs/Point"
	old_full_text = """
float64 x
float64 y
float64 z
"""

	new_type = "geometry_msgs/Point"
	new_full_text = """
float64 x
float64 y
float64 z
"""

	order = 0
	migrated_types = []

	valid = True

	def update(self, old_msg, new_msg):
		new_msg.x = old_msg.x
		new_msg.y = old_msg.y
		new_msg.z = old_msg.z

class update_robot_msgs_PointStamped_c63aecb41bfdfd6b7e1fac37c7cbe7bf(MessageUpdateRule):
	old_type = "robot_msgs/PointStamped"
	old_full_text = """
Header header
Point point

================================================================================
MSG: roslib/Header
#Standard metadata for higher-level flow data types
#sequence ID: consecutively increasing ID 
uint32 seq
#Two-integer timestamp that is expressed as:
# * stamp.secs: seconds (stamp_secs) since epoch
# * stamp.nsecs: nanoseconds since stamp_secs
# time-handling sugar is provided by the client library
time stamp
#Frame this data is associated with
# 0: no frame
# 1: global frame
string frame_id

================================================================================
MSG: robot_msgs/Point
float64 x
float64 y
float64 z
"""

	new_type = "geometry_msgs/PointStamped"
	new_full_text = """
Header header
Point point

================================================================================
MSG: roslib/Header
#Standard metadata for higher-level flow data types
#sequence ID: consecutively increasing ID 
uint32 seq
#Two-integer timestamp that is expressed as:
# * stamp.secs: seconds (stamp_secs) since epoch
# * stamp.nsecs: nanoseconds since stamp_secs
# time-handling sugar is provided by the client library
time stamp
#Frame this data is associated with
# 0: no frame
# 1: global frame
string frame_id

================================================================================
MSG: geometry_msgs/Point
float64 x
float64 y
float64 z
"""

	order = 0
	migrated_types = [
		("Header","Header"),
		("Point","Point"),]

	valid = True

	def update(self, old_msg, new_msg):
		self.migrate(old_msg.header, new_msg.header)
		self.migrate(old_msg.point, new_msg.point)

class update_robot_msgs_Transform_ac9eff44abf714214112b05d54a3cf9b(MessageUpdateRule):
	old_type = "robot_msgs/Transform"
	old_full_text = """
Vector3 translation
Quaternion rotation

================================================================================
MSG: robot_msgs/Vector3
float64 x
float64 y
float64 z
================================================================================
MSG: robot_msgs/Quaternion
# xyz - vector rotation axis, w - scalar term (cos(ang/2))
float64 x
float64 y
float64 z
float64 w
"""

	new_type = "geometry_msgs/Transform"
	new_full_text = """
Vector3 translation
Quaternion rotation

================================================================================
MSG: geometry_msgs/Vector3
float64 x
float64 y
float64 z
================================================================================
MSG: geometry_msgs/Quaternion
# xyz - vector rotation axis, w - scalar term (cos(ang/2))
float64 x
float64 y
float64 z
float64 w
"""

	order = 0
	migrated_types = [
		("Vector3","Vector3"),
		("Quaternion","Quaternion"),]

	valid = True

	def update(self, old_msg, new_msg):
		self.migrate(old_msg.translation, new_msg.translation)
		self.migrate(old_msg.rotation, new_msg.rotation)

class update_robot_msgs_TransformStamped_e0b1e16e4f459246e737dca6251c020b(MessageUpdateRule):
	old_type = "robot_msgs/TransformStamped"
	old_full_text = """
Header header
string parent_id # the frame id of the parent frame
Transform transform

================================================================================
MSG: roslib/Header
#Standard metadata for higher-level flow data types
#sequence ID: consecutively increasing ID 
uint32 seq
#Two-integer timestamp that is expressed as:
# * stamp.secs: seconds (stamp_secs) since epoch
# * stamp.nsecs: nanoseconds since stamp_secs
# time-handling sugar is provided by the client library
time stamp
#Frame this data is associated with
# 0: no frame
# 1: global frame
string frame_id

================================================================================
MSG: robot_msgs/Transform
Vector3 translation
Quaternion rotation

================================================================================
MSG: robot_msgs/Vector3
float64 x
float64 y
float64 z
================================================================================
MSG: robot_msgs/Quaternion
# xyz - vector rotation axis, w - scalar term (cos(ang/2))
float64 x
float64 y
float64 z
float64 w
"""

	new_type = "geometry_msgs/TransformStamped"
	new_full_text = """
Header header
string parent_id # the frame id of the parent frame
Transform transform

================================================================================
MSG: roslib/Header
#Standard metadata for higher-level flow data types
#sequence ID: consecutively increasing ID 
uint32 seq
#Two-integer timestamp that is expressed as:
# * stamp.secs: seconds (stamp_secs) since epoch
# * stamp.nsecs: nanoseconds since stamp_secs
# time-handling sugar is provided by the client library
time stamp
#Frame this data is associated with
# 0: no frame
# 1: global frame
string frame_id

================================================================================
MSG: geometry_msgs/Transform
Vector3 translation
Quaternion rotation

================================================================================
MSG: geometry_msgs/Vector3
float64 x
float64 y
float64 z
================================================================================
MSG: geometry_msgs/Quaternion
# xyz - vector rotation axis, w - scalar term (cos(ang/2))
float64 x
float64 y
float64 z
float64 w
"""

	order = 0
	migrated_types = [
		("Header","Header"),
		("Transform","Transform"),]

	valid = True

	def update(self, old_msg, new_msg):
		self.migrate(old_msg.header, new_msg.header)
		new_msg.parent_id = old_msg.parent_id
		self.migrate(old_msg.transform, new_msg.transform)

class update_robot_msgs_Pose_e45d45a5a1ce597b249e23fb30fc871f(MessageUpdateRule):
	old_type = "robot_msgs/Pose"
	old_full_text = """
Point position
Quaternion orientation

================================================================================
MSG: robot_msgs/Point
float64 x
float64 y
float64 z

================================================================================
MSG: robot_msgs/Quaternion
# xyz - vector rotation axis, w - scalar term (cos(ang/2))
float64 x
float64 y
float64 z
float64 w
"""

	new_type = "geometry_msgs/Pose"
	new_full_text = """
Point position
Quaternion orientation

================================================================================
MSG: geometry_msgs/Point
float64 x
float64 y
float64 z

================================================================================
MSG: geometry_msgs/Quaternion
# xyz - vector rotation axis, w - scalar term (cos(ang/2))
float64 x
float64 y
float64 z
float64 w
"""

	order = 0
	migrated_types = [
		("Point","Point"),
		("Quaternion","Quaternion"),]

	valid = True

	def update(self, old_msg, new_msg):
		self.migrate(old_msg.position, new_msg.position)
		self.migrate(old_msg.orientation, new_msg.orientation)

class update_robot_msgs_PoseStamped_d3812c3cbc69362b77dc0b19b345f8f5(MessageUpdateRule):
	old_type = "robot_msgs/PoseStamped"
	old_full_text = """
Header header
Pose pose

================================================================================
MSG: roslib/Header
#Standard metadata for higher-level flow data types
#sequence ID: consecutively increasing ID 
uint32 seq
#Two-integer timestamp that is expressed as:
# * stamp.secs: seconds (stamp_secs) since epoch
# * stamp.nsecs: nanoseconds since stamp_secs
# time-handling sugar is provided by the client library
time stamp
#Frame this data is associated with
# 0: no frame
# 1: global frame
string frame_id

================================================================================
MSG: robot_msgs/Pose
Point position
Quaternion orientation

================================================================================
MSG: robot_msgs/Point
float64 x
float64 y
float64 z

================================================================================
MSG: robot_msgs/Quaternion
# xyz - vector rotation axis, w - scalar term (cos(ang/2))
float64 x
float64 y
float64 z
float64 w
"""

	new_type = "geometry_msgs/PoseStamped"
	new_full_text = """
Header header
Pose pose

================================================================================
MSG: roslib/Header
#Standard metadata for higher-level flow data types
#sequence ID: consecutively increasing ID 
uint32 seq
#Two-integer timestamp that is expressed as:
# * stamp.secs: seconds (stamp_secs) since epoch
# * stamp.nsecs: nanoseconds since stamp_secs
# time-handling sugar is provided by the client library
time stamp
#Frame this data is associated with
# 0: no frame
# 1: global frame
string frame_id

================================================================================
MSG: geometry_msgs/Pose
Point position
Quaternion orientation

================================================================================
MSG: geometry_msgs/Point
float64 x
float64 y
float64 z

================================================================================
MSG: geometry_msgs/Quaternion
# xyz - vector rotation axis, w - scalar term (cos(ang/2))
float64 x
float64 y
float64 z
float64 w
"""

	order = 0
	migrated_types = [
		("Header","Header"),
		("Pose","Pose"),]

	valid = True

	def update(self, old_msg, new_msg):
		self.migrate(old_msg.header, new_msg.header)
		self.migrate(old_msg.pose, new_msg.pose)

class update_robot_msgs_Velocity_ffb367ff390f5e01cb55c0c75927c19a(MessageUpdateRule):
	old_type = "robot_msgs/Velocity"
	old_full_text = """
float64 vx
float64 vy
float64 vz
"""

	new_type = "geometry_msgs/Vector3"
	new_full_text = """
float64 x
float64 y
float64 z
"""

	order = 0
	migrated_types = []

	valid = True

	def update(self, old_msg, new_msg):
		new_msg.x = old_msg.vx
		new_msg.y = old_msg.vy
		new_msg.z = old_msg.vz

class update_robot_msgs_Point32_cc153912f1453b708d221682bc23d9ac(MessageUpdateRule):
	old_type = "robot_msgs/Point32"
	old_full_text = """
float32 x
float32 y
float32 z
"""

	new_type = "geometry_msgs/Point32"
	new_full_text = """
float32 x
float32 y
float32 z
"""

	order = 0
	migrated_types = []

	valid = True

	def update(self, old_msg, new_msg):
		new_msg.x = old_msg.x
		new_msg.y = old_msg.y
		new_msg.z = old_msg.z

class update_robot_msgs_Twist_104c6ef591961bed596cfa30f858271c(MessageUpdateRule):
	old_type = "robot_msgs/Twist"
	old_full_text = """
Header header
robot_msgs/Vector3  vel
robot_msgs/Vector3  rot

================================================================================
MSG: roslib/Header
#Standard metadata for higher-level flow data types
#sequence ID: consecutively increasing ID 
uint32 seq
#Two-integer timestamp that is expressed as:
# * stamp.secs: seconds (stamp_secs) since epoch
# * stamp.nsecs: nanoseconds since stamp_secs
# time-handling sugar is provided by the client library
time stamp
#Frame this data is associated with
# 0: no frame
# 1: global frame
string frame_id

================================================================================
MSG: robot_msgs/Vector3
float64 x
float64 y
float64 z
"""

	new_type = "geometry_msgs/TwistStamped"
	new_full_text = """
Header header
Twist twist

================================================================================
MSG: roslib/Header
#Standard metadata for higher-level flow data types
#sequence ID: consecutively increasing ID 
uint32 seq
#Two-integer timestamp that is expressed as:
# * stamp.secs: seconds (stamp_secs) since epoch
# * stamp.nsecs: nanoseconds since stamp_secs
# time-handling sugar is provided by the client library
time stamp
#Frame this data is associated with
# 0: no frame
# 1: global frame
string frame_id

================================================================================
MSG: geometry_msgs/Twist
Vector3  linear
Vector3  angular

================================================================================
MSG: geometry_msgs/Vector3
float64 x
float64 y
float64 z
"""

	order = 0
	migrated_types = [
		("Header","Header"),
                ("Vector3","Vector3"),]

	valid = True

	def update(self, old_msg, new_msg):
		self.migrate(old_msg.header, new_msg.header)
		new_msg.twist = self.get_new_class('Twist')()
                self.migrate(old_msg.vel, new_msg.twist.linear)
                self.migrate(old_msg.rot, new_msg.twist.angular)

class update_robot_msgs_Acceleration_271a8351f08a1993852d2e5e55f1efa1(MessageUpdateRule):
	old_type = "robot_msgs/Acceleration"
	old_full_text = """
float64 ax ##Axis angle format 
float64 ay ##Axis is defined by direction of x,y,z
float64 az ## magnitude in radians/second
"""

	new_type = "geometry_msgs/Vector3"
	new_full_text = """
float64 x
float64 y
float64 z
"""

	order = 0
	migrated_types = []

	valid = True

	def update(self, old_msg, new_msg):
		new_msg.x = old_msg.ax
		new_msg.y = old_msg.ay
		new_msg.z = old_msg.az


class update_robot_msgs_AngularAcceleration_271a8351f08a1993852d2e5e55f1efa1(MessageUpdateRule):
	old_type = "robot_msgs/AngularAcceleration"
	old_full_text = """
float64 ax ##Axis angle format 
float64 ay ##Axis is defined by direction of x,y,z
float64 az ## magnitude in radians/sec^2
"""

	new_type = "geometry_msgs/Vector3"
	new_full_text = """
float64 x
float64 y
float64 z
"""

	order = 0
	migrated_types = []

	valid = True

	def update(self, old_msg, new_msg):
		new_msg.x = old_msg.ax
		new_msg.y = old_msg.ay
		new_msg.z = old_msg.az

class update_robot_msgs_Polygon3D_290d2a9f8810ff59c2bf8ecc1fe478ec(MessageUpdateRule):
	old_type = "robot_msgs/Polygon3D"
	old_full_text = """
Point32[] points
std_msgs/ColorRGBA color

================================================================================
MSG: robot_msgs/Point32
float32 x
float32 y
float32 z
================================================================================
MSG: std_msgs/ColorRGBA
float32 r
float32 g
float32 b
float32 a
"""

	new_type = "geometry_msgs/Polygon"
	new_full_text = """
geometry_msgs/Point32[] points

================================================================================
MSG: geometry_msgs/Point32
float32 x
float32 y
float32 z
"""

	order = 0
	migrated_types = [
		("Point32","Point32"),]

	valid = True

	def update(self, old_msg, new_msg):
		self.migrate_array(old_msg.points, new_msg.points, "geometry_msgs/Point32")


class update_robot_msgs_PoseDot_a2c4de81995f15464ed26e3bae376045(MessageUpdateRule):
	old_type = "robot_msgs/PoseDot"
	old_full_text = """
Velocity vel
AngularVelocity ang_vel

================================================================================
MSG: robot_msgs/Velocity
float64 vx
float64 vy
float64 vz

================================================================================
MSG: robot_msgs/AngularVelocity
float64 vx ##Axis angle format 
float64 vy ##Axis is defined by direction of x,y,z
float64 vz ## magnitude in radians/second
"""

	new_type = "geometry_msgs/Twist"
	new_full_text = """
Vector3  linear
Vector3  angular

================================================================================
MSG: geometry_msgs/Vector3
float64 x
float64 y
float64 z
"""

	order = 0
	migrated_types = [('Velocity','Vector3'),
                          ('AngularVelocity','Vector3')]

	valid = True

	def update(self, old_msg, new_msg):
                self.migrate(old_msg.vel,     new_msg.linear)
                self.migrate(old_msg.ang_vel, new_msg.angular)


class update_robot_msgs_AngularVelocity_ffb367ff390f5e01cb55c0c75927c19a(MessageUpdateRule):
	old_type = "robot_msgs/AngularVelocity"
	old_full_text = """
float64 vx ##Axis angle format 
float64 vy ##Axis is defined by direction of x,y,z
float64 vz ## magnitude in radians/second
"""

	new_type = "geometry_msgs/Vector3"
	new_full_text = """
float64 x
float64 y
float64 z
"""

	order = 0
	migrated_types = []

	valid = True

	def update(self, old_msg, new_msg):
		new_msg.x = old_msg.vx
		new_msg.y = old_msg.vy
		new_msg.z = old_msg.vz


class update_robot_msgs_Wrench_31103fae9a9f6a2d32c8f5838aa25717(MessageUpdateRule):
	old_type = "robot_msgs/Wrench"
	old_full_text = """
Header header
robot_msgs/Vector3  force
robot_msgs/Vector3  torque

================================================================================
MSG: roslib/Header
#Standard metadata for higher-level flow data types
#sequence ID: consecutively increasing ID 
uint32 seq
#Two-integer timestamp that is expressed as:
# * stamp.secs: seconds (stamp_secs) since epoch
# * stamp.nsecs: nanoseconds since stamp_secs
# time-handling sugar is provided by the client library
time stamp
#Frame this data is associated with
# 0: no frame
# 1: global frame
string frame_id

================================================================================
MSG: robot_msgs/Vector3
float64 x
float64 y
float64 z
"""

	new_type = "geometry_msgs/WrenchStamped"
	new_full_text = """
Header header
Wrench data

================================================================================
MSG: roslib/Header
#Standard metadata for higher-level flow data types
#sequence ID: consecutively increasing ID 
uint32 seq
#Two-integer timestamp that is expressed as:
# * stamp.secs: seconds (stamp_secs) since epoch
# * stamp.nsecs: nanoseconds since stamp_secs
# time-handling sugar is provided by the client library
time stamp
#Frame this data is associated with
# 0: no frame
# 1: global frame
string frame_id

================================================================================
MSG: geometry_msgs/Wrench
Vector3  force
Vector3  torque

================================================================================
MSG: geometry_msgs/Vector3
float64 x
float64 y
float64 z
"""

	order = 0
	migrated_types = [
		("Header","Header"),
                ("Vector3","Vector3")]

	valid = True

	def update(self, old_msg, new_msg):
		self.migrate(old_msg.header, new_msg.header)
		self.migrate(old_msg.force, new_msg.data.force)
		self.migrate(old_msg.torque, new_msg.data.torque)

class update_robot_msgs_PoseWithCovariance_ecf54a1a25cdc75d7a5f2b4cddd77d27(MessageUpdateRule):
	old_type = "robot_msgs/PoseWithCovariance"
	old_full_text = """
Header header
robot_msgs/Pose pose

# Row-major representation of the 6x6 covariance matrix
# (x, y, z, EulerZ, EulerY, EulerX)
float64[36] covariance
================================================================================
MSG: roslib/Header
#Standard metadata for higher-level flow data types
#sequence ID: consecutively increasing ID 
uint32 seq
#Two-integer timestamp that is expressed as:
# * stamp.secs: seconds (stamp_secs) since epoch
# * stamp.nsecs: nanoseconds since stamp_secs
# time-handling sugar is provided by the client library
time stamp
#Frame this data is associated with
# 0: no frame
# 1: global frame
string frame_id

================================================================================
MSG: robot_msgs/Pose
Point position
Quaternion orientation

================================================================================
MSG: robot_msgs/Point
float64 x
float64 y
float64 z

================================================================================
MSG: robot_msgs/Quaternion
# xyz - vector rotation axis, w - scalar term (cos(ang/2))
float64 x
float64 y
float64 z
float64 w
"""

	new_type = "geometry_msgs/PoseWithCovarianceStamped"
	new_full_text = """
Header header
PoseWithCovariance pose

================================================================================
MSG: roslib/Header
#Standard metadata for higher-level flow data types
#sequence ID: consecutively increasing ID 
uint32 seq
#Two-integer timestamp that is expressed as:
# * stamp.secs: seconds (stamp_secs) since epoch
# * stamp.nsecs: nanoseconds since stamp_secs
# time-handling sugar is provided by the client library
time stamp
#Frame this data is associated with
# 0: no frame
# 1: global frame
string frame_id

================================================================================
MSG: geometry_msgs/PoseWithCovariance
Pose pose

# Row-major representation of the 6x6 covariance matrix
# The orientation parameters use a fixed-axis representation.
# In order, the parameters are:
# (x, y, z, rotation about Z axis, rotation about Y axis, rotation about X axis)
float64[36] covariance

================================================================================
MSG: geometry_msgs/Pose
Point position
Quaternion orientation

================================================================================
MSG: geometry_msgs/Point
float64 x
float64 y
float64 z

================================================================================
MSG: geometry_msgs/Quaternion
# xyz - vector rotation axis, w - scalar term (cos(ang/2))
float64 x
float64 y
float64 z
float64 w
"""

	order = 0
	migrated_types = [
		("Header","Header"),
                ("Pose","Pose")]

	valid = True

	def update(self, old_msg, new_msg):
		self.migrate(old_msg.header, new_msg.header)
                self.migrate(old_msg.pose, new_msg.pose.pose)
                new_msg.pose.covariance = old_msg.covariance


