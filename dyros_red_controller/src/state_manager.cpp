#include "dyros_red_controller/state_manager.h"
#include <ros/package.h>
#include <rbdl/rbdl.h>
#include <rbdl/addons/urdfreader/urdfreader.h>
#include <tf/transform_datatypes.h>
#include <sstream>

StateManager::StateManager(DataContainer &dc_global) : dc(dc_global)
{
    gui_command = dc.nh.subscribe("/dyros_red/command", 100, &StateManager::CommandCallback, this);

    joint_states_pub = dc.nh.advertise<sensor_msgs::JointState>("/dyros_red/jointstates", 1);
    time_pub = dc.nh.advertise<std_msgs::Float32>("/dyros_red/time", 1);
    motor_acc_dif_info_pub = dc.nh.advertise<dyros_red_msgs::MotorInfo>("/dyros_red/accdifinfo", 1);
    tgainPublisher = dc.nh.advertise<std_msgs::Float32>("/dyros_red/torquegain", 100);
    point_pub = dc.nh.advertise<geometry_msgs::PolygonStamped>("/dyros_red/point", 3);

    pointpub_msg.polygon.points.resize(3);

    if (dc.mode == "realrobot")
    {
        motor_info_pub = dc.nh.advertise<dyros_red_msgs::MotorInfo>("/dyros_red/motorinfo", 1);
        motor_info_msg.motorinfo1.resize(MODEL_DOF);
        motor_info_msg.motorinfo2.resize(MODEL_DOF);
    }
    acc_dif_info_msg.motorinfo1.resize(MODEL_DOF);
    acc_dif_info_msg.motorinfo2.resize(MODEL_DOF);

    joint_state_msg.position.resize(MODEL_DOF);
    joint_state_msg.velocity.resize(MODEL_DOF);
    joint_state_msg.effort.resize(MODEL_DOF);

    gravity_.setZero();
    gravity_(2) = GRAVITY;

    initialize();
    bool verbose = false; //set verbose true for State Manager initialization info

    std::string desc_package_path = ros::package::getPath("dyros_red_lowerbody_description");
    std::string urdf_path = desc_package_path + "/robots/red_robot_lowerbody.urdf";

    ROS_INFO_COND(verbose, "Loading DYROS JET description from = %s", urdf_path.c_str());

    RigidBodyDynamics::Addons::URDFReadFromFile(urdf_path.c_str(), &model_, true, verbose);

    ROS_INFO_COND(verbose, "Successfully loaded.");
    ROS_INFO_COND(verbose, "MODEL DOF COUNT = %d and MODEL Q SIZE = %d ", model_.dof_count, model_.q_size);

    // model_.mJoints[0].)
    if (model_.dof_count != MODEL_DOF + 6)
    {
        ROS_WARN("The DoF in the model file and the code do not match.");
        ROS_WARN("Model file = %d, Code = %d", model_.dof_count, (int)MODEL_DOF + 6);
    }
    else
    {
        // ROS_INFO("id:0 name is : %s",model_.GetBodyName(0));
        for (int i = 0; i < MODEL_DOF + 1; i++)
        {
            link_id_[i] = model_.GetBodyId(RED::LINK_NAME[i]);
            if (!model_.IsBodyId(link_id_[i]))
            {
                ROS_INFO_COND(verbose, "Failed to get body id at link %d : %s", i, RED::LINK_NAME[i]);
            }
            // ROS_INFO("%s: \t\t id = %d \t parent link = %d",LINK_NAME[i],
            // link_id_[i],model_.GetParentBodyId(link_id_[i]));
            // ROS_INFO("%dth parent
            // %d",link_id_[i],model_.GetParentBodyId(link_id_[i]));
            // std::cout << model_.mBodies[link_id_[i]].mCenterOfMass << std::endl;
            // //joint_name_map_[JOINT_NAME[i]] = i;
        }

        for (int i = 0; i < MODEL_DOF + 1; i++)
        {
            link_[i].initialize(model_, link_id_[i], RED::LINK_NAME[i], model_.mBodies[link_id_[i]].mMass, model_.mBodies[link_id_[i]].mCenterOfMass);
        }

        Eigen::Vector3d lf_c, rf_c, lh_c, rh_c;
        lf_c << 0.0317, 0, -0.1368;
        rf_c << 0.0317, 0, -0.1368;
        link_[Right_Foot].contact_point = rf_c;
        link_[Left_Foot].contact_point = lf_c;

        joint_state_msg.name.resize(MODEL_DOF);
        for (int i = 0; i < MODEL_DOF; i++)
        {
            joint_state_msg.name[i] = RED::JOINT_NAME[i];
        }
        // RigidBodyDynamics::Joint J_temp;
        // J_temp=RigidBodyDynamics::Joint(RigidBodyDynamics::JointTypeEulerXYZ);
        // model_.mJoints[2] = J_temp;
    }

    ROS_INFO_COND(verbose, "State manager Init complete");
}

void StateManager::stateThread(void)
{
    std::chrono::high_resolution_clock::time_point StartTime = std::chrono::high_resolution_clock::now();
    //std::chrono::high_resolution_clock::time_point StartTime2 = std::chrono::high_resolution_clock::now();
    std::chrono::seconds sec10(1);
    std::chrono::milliseconds ms(50);

    std::chrono::high_resolution_clock::time_point int_StartTime = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> e_s(0);
    //ROS_INFO("START");
    int ThreadCount = 0;
    int i = 1;
    int dcount = 0;
    int ThreadCount2 = 0;

    while (ros::ok())
    {

        //std::cout << ThreadCount<<" : update state, ";
        updateState();

        //std::cout << "update rbdl, ";
        updateKinematics(q_virtual_, q_dot_virtual_, q_ddot_virtual_);

        //std::cout <<" state estimation ";
        stateEstimate();

        updateKinematics(q_virtual_, q_dot_virtual_, q_ddot_virtual_);

        //std::cout << "store states, ";
        storeState();

        //std::cout << "store states complete !" <<std::endl;
        dc.firstcalcdone = true;

        if (dc.shutdown)
        {
            break;
        }
        e_s = int_StartTime - StartTime;
        //To check frequency
        if (e_s > sec10)
        {
            StartTime = std::chrono::high_resolution_clock::now();
            //rprint(dc, 0, 0, "s count : %d", ThreadCount - (i - 1) * 4000);

            std::cout << control_time_ << " ::: state thread : " << ThreadCount - ThreadCount2 << " hz ";
            //<< std::endl;
            ThreadCount2 = ThreadCount;
            i++;
        }

        //Data to GUI
        if ((ThreadCount % (int)(dc.stm_hz / 60)) == 0)
        {
            joint_state_msg.header.stamp = ros::Time::now();
            for (int i = 0; i < MODEL_DOF; i++)
            {
                joint_state_msg.position[i] = q_[i];
                joint_state_msg.velocity[i] = q_dot_[i];
                joint_state_msg.effort[i] = torque_desired[i];
            }
            joint_states_pub.publish(joint_state_msg);
            time_msg.data = control_time_;
            time_pub.publish(time_msg);
            if (dc.mode == "realrobot")
            {
                for (int i = 0; i < MODEL_DOF; i++)
                {
                    motor_info_msg.motorinfo1[i] = dc.torqueElmo[i];
                    motor_info_msg.motorinfo2[i] = dc.torqueDemandElmo[i];
                }
                motor_info_pub.publish(motor_info_msg);
            }
            tgain_p.data = dc.t_gain;
            tgainPublisher.publish(tgain_p);

            point_pub.publish(pointpub_msg);
        }

        //every 1 seconds
        if ((ThreadCount % (int)(dc.stm_hz)) == 0)
        {
            //std::cout << "data received : " << data_received_counter_ - dcount << std::endl;
            dcount = data_received_counter_;
        }

        for (int i = 0; i < MODEL_DOF; i++)
        {
            acc_dif_info_msg.motorinfo1[i] = dc.accel_dif[i];
            acc_dif_info_msg.motorinfo2[i] = dc.accel_obsrvd[i];
        }

        motor_acc_dif_info_pub.publish(acc_dif_info_msg);

        ThreadCount++;

        //std::this_thread::sleep_until(int_StartTime + dc.stm_timestep);
        //std::this_thread::sleep_until(int_StartTime + dc.stm_timestep);
        int_StartTime = std::chrono::high_resolution_clock::now();
    }
}
void StateManager::testThread()
{
    std::chrono::high_resolution_clock::time_point StartTime = std::chrono::high_resolution_clock::now();
    std::chrono::seconds sec10(1);
    std::chrono::milliseconds ms(50);

    std::chrono::duration<double> e_s(0);
    //ROS_INFO("START");
    int ThreadCount = 0;

    while (ros::ok())
    {

        updateState();
        updateKinematics(q_virtual_, q_dot_virtual_, q_ddot_virtual_);

        stateEstimate();
        updateKinematics(q_virtual_, q_dot_virtual_, q_ddot_virtual_);

        storeState();

        //std::this_thread::sleep_until(StartTime + ThreadCount * dc.stm_timestep);
        if ((ThreadCount % 2000) == 0)
        {
            e_s = std::chrono::high_resolution_clock::now() - StartTime;
            rprint(dc, 19, 10, "Kinematics update %8.4f hz                         ", 2000 / e_s.count());
            StartTime = std::chrono::high_resolution_clock::now();
        }
        if (dc.shutdown)
        {
            rprint(dc, true, 19, 10, "state end");
            break;
        }
        ThreadCount++;
    }
}
void StateManager::connect()
{
    //overrid
}
void StateManager::updateState()
{
    //overrid by simulation or red robot
}

void StateManager::sendCommand(Eigen::VectorQd command, double simt)
{
    //overrid by simulation or red robot
}

void StateManager::initialize()
{
    data_received_counter_ = 0;
    A_.setZero();
    A_temp_.setZero(MODEL_DOF_VIRTUAL, MODEL_DOF_VIRTUAL);
    q_.setZero();
    q_dot_.setZero();

    q_virtual_.setZero();
    q_dot_virtual_.setZero();
    q_ddot_virtual_.setZero();

    torque_desired.setZero();

    dc.torqueElmo.setZero();
}

void StateManager::storeState()
{
    mtx_dc.lock();

    for (int i = 0; i < LINK_NUMBER + 1; i++)
    {
        dc.link_[i] = link_[i];
    }
    dc.com_ = com_;
    dc.time = control_time_;
    dc.sim_time = sim_time_;

    dc.q_ = q_;
    dc.q_dot_ = q_dot_;
    dc.q_dot_virtual_ = q_dot_virtual_;
    dc.q_virtual_ = q_virtual_;
    dc.q_ddot_virtual_ = q_ddot_virtual_;

    dc.yaw_radian = yaw_radian;
    dc.A_ = A_;
    dc.A_inv = A_inv;

    mtx_dc.unlock();
}

void StateManager::updateKinematics(const Eigen::VectorXd &q_virtual, const Eigen::VectorXd &q_dot_virtual, const Eigen::VectorXd &q_ddot_virtual)
{
    //ROS_INFO_ONCE("CONTROLLER : MODEL : updatekinematics enter ");
    /* q_virtual description
   * 0 ~ 2 : XYZ cartesian coordinates
   * 3 ~ 5 : XYZ Quaternion
   * 6 ~ MODEL_DOF + 5 : joint position
   * model dof + 6 ( last component of q_virtual) : w of Quaternion
   * */
    mtx_rbdl.lock();
    RigidBodyDynamics::UpdateKinematicsCustom(model_, &q_virtual, &q_dot_virtual, &q_ddot_virtual);
    RigidBodyDynamics::CompositeRigidBodyAlgorithm(model_, q_virtual_, A_temp_, false);
    mtx_rbdl.unlock();

    tf::Quaternion q(q_virtual_(3), q_virtual_(4), q_virtual_(5), q_virtual_(MODEL_DOF + 6));

    tf::Matrix3x3 m(q);
    double roll, pitch, yaw;
    m.getRPY(roll, pitch, yaw);
    yaw_radian = yaw;

    A_ = A_temp_;
    A_inv = A_.inverse();

    for (int i = 0; i < MODEL_DOF + 1; i++)
    {
        link_[i].pos_Update(model_, q_virtual_);
    }

    Eigen::Vector3d zero;
    zero.setZero();
    dc.check = true;
    for (int i = 0; i < MODEL_DOF + 1; i++)
    {
        link_[i].Set_Jacobian(model_, q_virtual_, zero);
    }
    dc.check = false;

    for (int i = 0; i < MODEL_DOF + 1; i++)
    {

        link_[i].COM_Jac_Update(model_, q_virtual_);
    }
    //COM link information update ::
    double com_mass;
    RigidBodyDynamics::Math::Vector3d com_pos;
    RigidBodyDynamics::Math::Vector3d com_vel, com_accel, com_ang_momentum;
    mtx_rbdl.lock();
    RigidBodyDynamics::Utils::CalcCenterOfMass(model_, q_virtual_, q_dot_virtual_, &q_ddot_virtual, com_mass, com_pos, &com_vel, &com_accel, &com_ang_momentum, NULL, false);
    mtx_rbdl.unlock();

    com_.mass = com_mass;
    com_.pos = com_pos;

    pointpub_msg.polygon.points[0].x = com_pos(0);
    pointpub_msg.polygon.points[0].y = com_pos(1);
    pointpub_msg.polygon.points[0].z = com_pos(2);

    pointpub_msg.polygon.points[1].x = link_[Right_Foot].xpos(0);
    pointpub_msg.polygon.points[1].y = link_[Right_Foot].xpos(1);
    pointpub_msg.polygon.points[1].z = link_[Right_Foot].xpos(2);

    pointpub_msg.polygon.points[2].x = link_[Left_Foot].xpos(0);
    pointpub_msg.polygon.points[2].y = link_[Left_Foot].xpos(1);
    pointpub_msg.polygon.points[2].z = link_[Left_Foot].xpos(2);

    /*
    if (com_pos(1) < link_[Right_Foot].xpos(1))
    {
        std::cout << control_time_ << "COM_Y OUT WARNING !!!!!!!!!!!!!!!!" << std::endl;
    }
    else if (com_pos(1) > link_[Left_Foot].xpos(1))
    {
        std::cout << control_time_ << "COM_Y OUT WARNING !!!!!!!!!!!!!!!!" << std::endl;
    }
    if ((com_pos(0) < link_[Right_Foot].xpos(0)) && (com_pos(0) < link_[Left_Foot].xpos(0)))
    {
        std::cout << control_time_ << "COM_Y OUT WARNING !!!!!!!!!!!!!!!!" << std::endl;
    }
    else if (com_pos(0) > link_[Left_Foot].xpos(0))
    {
        std::cout << control_time_ << "COM_Y OUT WARNING !!!!!!!!!!!!!!!!" << std::endl;
    } */

    Eigen::Vector3d foot_ahead_pos(0.15, 0, 0);
    Eigen::Vector3d foot_back_pos(-0.09, 0, 0);
    Eigen::Vector3d RH, RT, LH, LT;

    RH = link_[Right_Foot].xpos + link_[Right_Foot].Rotm * foot_ahead_pos;
    RT = link_[Right_Foot].xpos + link_[Right_Foot].Rotm * foot_back_pos;

    LH = link_[Left_Foot].xpos + link_[Left_Foot].Rotm * foot_ahead_pos;
    LT = link_[Left_Foot].xpos + link_[Left_Foot].Rotm * foot_back_pos;

    double s[4];

    s[0] = DyrosMath::check_border(com_.pos(0), com_.pos(1), RH(0), RT(0), RH(1), RT(1), -1.0);
    s[1] = DyrosMath::check_border(com_.pos(0), com_.pos(1), RT(0), LT(0), RT(1), LT(1), -1.0);
    s[2] = DyrosMath::check_border(com_.pos(0), com_.pos(1), LT(0), LH(0), LT(1), LH(1), -1.0);
    s[3] = DyrosMath::check_border(com_.pos(0), com_.pos(1), LH(0), RH(0), LH(1), RH(1), -1.0);
    //std::cout << " com pos : x " << com_.pos(0) << "\t" << com_.pos(1) << std::endl;
    //std::cout << "check sign ! \t" << s[0] << "\t" << s[1] << "\t" << s[2] << "\t" << s[3] << std::endl;

    for (int i = 0; i < 4; i++)
    {
        if (s[i] < 0)
        {
            if (dc.spalarm)
                std::cout << control_time_ << "com is out of support polygon !, line " << i << std::endl;
        }
    }

    /*
    s[1] = DyrosMath::check_border(com_.pos(0), com_.pos(1), RT(0), LT(0), RT(1), LT(1), 1.0);
    std::cout << " s[1] : " << s[1] << std::endl; */
    Eigen::Vector3d vel_temp;
    vel_temp = com_.vel;
    com_.vel = com_vel;

    com_.accel = -com_accel;
    com_.angular_momentum = com_ang_momentum;

    double w_ = sqrt(9.81 / com_.pos(2));

    com_.ZMP(0) = com_.pos(0) - com_.accel(0) / pow(w_, 2);
    com_.ZMP(1) = com_.pos(1) - com_.accel(1) / pow(w_, 2);

    //com_.ZMP(0) = (com_.pos(0) * (com_.accel(2) + 9.81) - com_pos(2) * com_accel(0)) / (com_.accel(2) + 9.81) - com_.angular_momentum(2) / com_.mass / (com_.accel(2) + 9.81);
    //com_.ZMP(1) = (com_.pos(1) * (com_.accel(2) + 9.81) - com_pos(2) * com_accel(1)) / (com_.accel(2) + 9.81) - com_.angular_momentum(1) / com_.mass / (com_.accel(2) + 9.81);

    com_.CP(0) = com_.pos(0) + com_.vel(0) / w_;
    com_.CP(1) = com_.pos(1) + com_.vel(1) / w_;

    Eigen::MatrixXd jacobian_com;
    jacobian_com.setZero(3, MODEL_DOF + 6);

    for (int i = 0; i < LINK_NUMBER; i++)
    {
        jacobian_com += link_[i].Jac_COM_p * link_[i].Mass;
    }
    jacobian_com = jacobian_com / com_.mass;

    link_[COM_id].Jac.setZero(6, MODEL_DOF + 6);

    link_[COM_id].Jac.block(0, 0, 2, MODEL_DOF + 6) = jacobian_com.block(0, 0, 2, MODEL_DOF + 6);
    link_[COM_id].Jac.block(2, 0, 4, MODEL_DOF + 6) = link_[Pelvis].Jac.block(2, 0, 4, MODEL_DOF + 6);
    link_[COM_id].xpos = com_.pos;
    link_[COM_id].xpos(2) = link_[Pelvis].xpos(2);
    link_[COM_id].Rotm = link_[Pelvis].Rotm;

    for (int i = 0; i < LINK_NUMBER + 1; i++)
    {
        link_[i].vw_Update(q_dot_virtual_);
    }

    //contactJacUpdate
    //link_[Right_Foot].Set_Contact(model_, q_virtual_, link_[Right_Foot].contact_point);
    //link_[Left_Foot].Set_Contact(model_, q_virtual_, link_[Left_Foot].contact_point);
    //link_[Right_Hand].Set_Contact(model_, q_virtual_, link_[Right_Hand].contact_point);
    //link_[Left_Hand].Set_Contact(model_, q_virtual_, link_[Left_Hand].contact_point);
    //ROS_INFO_ONCE("CONTROLLER : MODEL : updatekinematics end ");
}

void StateManager::stateEstimate()
{
}

void StateManager::CommandCallback(const std_msgs::StringConstPtr &msg)
{
    //std::cout << "msg from gui : " << msg->data << std::endl;
    dc.command = msg->data;

    if (msg->data == "torqueon")
    {
        if (dc.torqueOn)
        {
            std::cout << "torque is already enabled, command duplicated, ignoring command!" << std::endl;
        }
        else
        {
            std::cout << "torque ON !" << std::endl;
            dc.torqueOnTime = control_time_;
            dc.torqueOn = true;
            dc.torqueOff = false;
        }
    }
    else if (msg->data == "positioncontrol")
    {
        if (!dc.positionControl)
        {
            std::cout << "Joint position control : on " << std::endl;
            dc.commandTime = control_time_;
            dc.positionDesired = q_;
        }
        else
        {
            std::cout << "Joint position control : off " << std::endl;
        }

        dc.positionControl = !dc.positionControl;
    }
    else if (msg->data == "torqueoff")
    {
        if (dc.torqueOn)
        {
            std::cout << "Torque OFF ! " << std::endl;
            dc.torqueOffTime = control_time_;
            dc.torqueOn = false;
            dc.torqueOff = true;
        }
        else if (dc.torqueOff)
        {
            std::cout << "Torque is already disabled, command duplicated, ignoring command! " << std::endl;
        }
    }
    else if (msg->data == "gravity")
    {
        if (dc.gravityMode)
        {
            std::cout << "gravity compensation mode : off " << std::endl;
            dc.gravityMode = false;
        }
        else
        {
            std::cout << "gravity compensation mode is on! " << std::endl;
            dc.commandTime = control_time_;
            dc.gravityMode = true;
        }
    }
    else if (msg->data == "emergencyoff")
    {
        std::cout << "!!!!!! RED DISABLED BY EMERGENCY OFF BUTTON !!!!!!" << std::endl;
        dc.emergencyoff = true;
        dc.torqueOn = false;
        dc.torqueOff = true;
    }
    else if (msg->data == "tunereset")
    {
        std::cout << "custom gain disabled, defualt gain enabled!" << std::endl;
        dc.customGain = false;
    }
    else if (msg->data == "tunecurrent")
    {
        std::cout << "default motor gain is : " << std::endl;
        for (int i = 0; i < MODEL_DOF; i++)
        {
            std::cout << dc.currentGain(i) << "\t";
        }
        std::cout << std::endl;
    }
    else if (msg->data == "fixedgravity")
    {
        std::cout << "fixed based gravity compensation mode" << std::endl;
        dc.fixedgravity = true;
    }
    else if (msg->data == "spalarm")
    {
        if (dc.spalarm)
            std::cout << "Support Polygon alarm : Off" << std::endl;
        else
            std::cout << "Support Polygon alarm : On" << std::endl;

        dc.spalarm = !dc.spalarm;
    }
    else if (msg->data == "torqueredis")
    {
        if (dc.torqueredis)
        {
            std::cout << "Torque contact redistribution off " << std::endl;
        }
        else
        {
            std::cout << "Torque contact redistribution on " << std::endl;
        }
        dc.torqueredis = !dc.torqueredis;
    }
}
