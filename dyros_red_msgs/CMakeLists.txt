cmake_minimum_required(VERSION 2.8.3)
project(dyros_red_msgs)

find_package(catkin REQUIRED COMPONENTS
  message_generation
  geometry_msgs
  sensor_msgs
  std_msgs
)


add_message_files(
  FILES
  JointSet.msg
  JointState.msg
  JointCommand.msg
  TaskCommand.msg
  WalkingCommand.msg
  ComCommand.msg
  Dynamicsinfo.msg
  GainCommand.msg
  MotorInfo.msg
)


generate_messages(
  DEPENDENCIES std_msgs geometry_msgs
)

catkin_package(
  CATKIN_DEPENDS message_runtime std_msgs
  geometry_msgs
  sensor_msgs
)

include_directories(
  ${catkin_INCLUDE_DIRS}
)
