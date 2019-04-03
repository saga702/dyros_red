#include "dyros_red_controller/state_manager.h"
#include <ros/package.h>
#include <rbdl/rbdl.h>
#include <rbdl/addons/urdfreader/urdfreader.h>
#include <tf/transform_datatypes.h>

std::mutex mtx;

StateManager::StateManager(DataContainer &dc_global) : dc(dc_global)
{
    gravity_.setZero();
    gravity_(2) = GRAVITY;

    initialize();
    bool verbose = false;

    std::string desc_package_path = ros::package::getPath("dyros_red_description");
    std::string urdf_path = desc_package_path + "/robots/dyros_red_robot.urdf";

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
            // ROS_INFO("%s: \t\t id = %d \t parent link = %d",LINK_NAME[i],
            // link_id_[i],model_.GetParentBodyId(link_id_[i]));
            // ROS_INFO("%dth parent
            // %d",link_id_[i],model_.GetParentBodyId(link_id_[i]));
            // std::cout << model_.mBodies[link_id_[i]].mCenterOfMass << std::endl;
            // //joint_name_map_[JOINT_NAME[i]] = i;
        }
        for (int i = 0; i < MODEL_DOF + 1; i++)
        {
            link_[i].initialize(link_id_[i], RED::LINK_NAME[i], model_.mBodies[link_id_[i]].mMass, model_.mBodies[link_id_[i]].mCenterOfMass);
        }

        Eigen::Vector3d lf_c, rf_c, lh_c, rh_c;
        lf_c << 0.0317, 0, -0.1368;
        rf_c << 0.0317, 0, -0.1368;
        rh_c << 0, -0.092, 0;
        lh_c << 0, 0.092, 0;
        link_[Right_Foot].contact_point = rf_c;
        link_[Left_Foot].contact_point = lf_c;
        link_[Right_Hand].contact_point = rh_c;
        link_[Left_Hand].contact_point = lh_c;

        // RigidBodyDynamics::Joint J_temp;
        // J_temp=RigidBodyDynamics::Joint(RigidBodyDynamics::JointTypeEulerXYZ);
        // model_.mJoints[2] = J_temp;
    }
}

void StateManager::stateThread(void)
{
    connect();

    std::chrono::high_resolution_clock::time_point StartTime = std::chrono::high_resolution_clock::now();
    std::chrono::seconds sec10(1);

    //ROS_INFO("START");
    int ThreadCount = 0;

    while (ros::ok())
    {

        updateState();
        updateKinematics(q_virtual_, q_dot_virtual_, q_ddot_virtual_);

        mtx.lock();
        storeState();
        mtx.unlock();

        //std::this_thread::sleep_until(StartTime + ThreadCount * dc.stm_timestep);
        if ((std::chrono::high_resolution_clock::now() - StartTime) > sec10)
        {
            //ROS_INFO("END");
            int w_x, w_y;
            getyx(stdscr, w_y, w_x);
            mvprintw(w_y + 2, 10, "Total Calc Count : %d times in 1 second! ", ThreadCount);
            refresh();
            //std::cout << "\n\tTotal Calc Count : " << ThreadCount << " times in 1 second! \n";
            //std::cout << link_[0];
            break;
        }

        ThreadCount++;
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

        mtx.lock();
        storeState();
        mtx.unlock();

        //std::this_thread::sleep_until(StartTime + ThreadCount * dc.stm_timestep);
        if ((ThreadCount % 500) == 0)
        {
            mtx.lock();
            e_s = std::chrono::high_resolution_clock::now() - StartTime;
            mvprintw(19, 10, "Kinematics update %8.4f hz                         ", 500 / e_s.count());
            refresh();
            //std::cout << "\n\tTotal Calc Count : " << ThreadCount << " times in 1 second! \n";
            //std::cout << link_[0];
            StartTime = std::chrono::high_resolution_clock::now();
            mtx.unlock();
        }
        if (!(getch() == -1))
        {
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

void StateManager::sendCommand(Eigen::VectorQd command)
{
    //overrid by simulation or red robot
}

void StateManager::initialize()
{
    A_.setZero();
    A_temp_.setZero(37, 37);
    q_.setZero();
    q_dot_.setZero();

    q_virtual_.setZero();
    q_dot_virtual_.setZero();
    q_ddot_virtual_.setZero();
}

void StateManager::storeState()
{
    for (int i = 0; i < LINK_NUMBER; i++)
    {
        dc.link_[i] = link_[i];
    }
    dc.com_ = com_;
    dc.q_ = q_;
    dc.q_dot_ = q_dot_;
    dc.q_dot_virtual_ = q_dot_virtual_;
    dc.q_virtual_ = q_virtual_;
    dc.q_ddot_virtual_ = q_ddot_virtual_;

    dc.yaw_radian = yaw_radian;
    dc.A_ = A_;
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
    RigidBodyDynamics::UpdateKinematicsCustom(model_, &q_virtual, &q_dot_virtual, &q_ddot_virtual);

    RigidBodyDynamics::CompositeRigidBodyAlgorithm(model_, q_virtual_, A_temp_, true);

    tf::Quaternion q(q_virtual_(3), q_virtual_(4), q_virtual_(5), q_virtual_(MODEL_DOF + 6));

    tf::Matrix3x3 m(q);
    double roll, pitch, yaw;
    m.getRPY(roll, pitch, yaw);
    yaw_radian = yaw;

    A_ = A_temp_;
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

    RigidBodyDynamics::Utils::CalcCenterOfMass(model_, q_virtual_, q_dot_virtual_, &q_ddot_virtual, com_mass, com_pos, &com_vel, &com_accel, &com_ang_momentum, NULL, true);

    com_.mass = com_mass;
    com_.pos = com_pos;

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

    //ROS_INFO_ONCE("CONTROLLER : MODEL : updatekinematics end ");
}