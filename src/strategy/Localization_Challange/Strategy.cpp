/**
 * @file Strategy.cpp
 *
 * @brief Localization challange pathplan strategy
 *
 * @date July 2017
 **/
/*****************************************************************************
** Includes
*****************************************************************************/
#include "Strategy.hpp"
Strategy::Strategy()
{
    _LocationState = forward;
    _Last_state = turn;
    _CurrentTarget = 0;
    _Location = new LocationStruct;
    _Env = new Environment;
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < 5; j++)
            through_path_ary[i][j] = 0;
}
void Strategy::setParam(Parameter *Param)
{
    _Param = Param;
}
void Strategy::GameState(int int_GameState)
{
    switch (int_GameState)
    {
    case STATE_HALT:
        StrategyHalt();
        break;
    case STATE_LOCALIZATION:
        StrategyLocalization();
        // StrategyLocalization2();
        break;
    }
}
void Strategy::StrategyHalt()
{
    _Env->Robot.v_x = 0;
    _Env->Robot.v_y = 0;
    _Env->Robot.v_yaw = 0;
}
void Strategy::StrategyLocalization()
{
    RobotData Robot;
    Robot.pos.x = _Env->Robot.pos.x;
    Robot.pos.y = _Env->Robot.pos.y;
    double imu = _Env->Robot.pos.angle;
    double absolute_front = imu + 90;
    static int flag = TRUE;
    static int flag_chase = TRUE;
    double ball_dis = 0.22;            // if you don't want to get information by vision
    double ball_angle = 0.0;          // if you don't want to get information by vision
    // double ball_dis = _Env->Robot.ball.distance;     //get ball information by vision
    // double ball_angle = _Env->Robot.ball.angle;      //get ball information by vision
    double lost_ball_dis = _Param->Strategy.HoldBall_Condition[3];
    double lost_ball_angle = _Param->Strategy.HoldBall_Condition[2];
    double hold_ball_dis = _Param->Strategy.HoldBall_Condition[1];
    double hold_ball_angle = _Param->Strategy.HoldBall_Condition[0];
    static double Begin_time = ros::Time::now().toSec(); // init timer begin
    double Current_time = ros::Time::now().toSec();      // init timer end
    double v_x, v_y, v_yaw;
    static int IMU_state = 0;
    double v_x_temp, v_y_temp;
    double fector;
    Robot.ball.x = Robot.pos.x + ball_dis * cos((absolute_front + ball_angle) * DEG2RAD);
    Robot.ball.y = Robot.pos.y + ball_dis * sin((absolute_front + ball_angle) * DEG2RAD);
    Vector3D vector_turn, vector_tr;
    double turn_yaw;
    
    Normalization(absolute_front);
    switch (_LocationState)
    {
    case forward: // Move to target poitn
        _Last_state = _LocationState;
        flag = FALSE;
        v_x = (_Location->LocationPoint[_CurrentTarget].x) - Robot.ball.x;
        v_y = (_Location->LocationPoint[_CurrentTarget].y) - Robot.ball.y;
        v_x_temp = v_x * cos((-imu) * DEG2RAD) - v_y * sin((-imu) * DEG2RAD);
        v_y_temp = v_x * sin((-imu) * DEG2RAD) + v_y * cos((-imu) * DEG2RAD);
        Current_time = ros::Time::now().toSec();
        fector = (Current_time - Begin_time)*2/3;
        if(fector>=1)
            fector = 1;
        v_x = v_x_temp*fector;
        v_y = v_y_temp*fector;
        if(fector >= 1){
            if(fabs(v_x) <= 0.05)
                v_x = 0;
            else if(fabs(v_y) <= 0.05)
                v_y = 0;
            if (fabs(v_x) <= 0.05 && fabs(v_y) <= 0.05)
            {
                _LocationState = reset_timer;
                flag = TRUE;
            }
        }
        break;
    case back: // Back to middle circle
        _Last_state = _LocationState;
        flag = FALSE;
        v_x = (_Location->MiddlePoint[_CurrentTarget].x) - Robot.ball.x;
        v_y = (_Location->MiddlePoint[_CurrentTarget].y) - Robot.ball.y;
        v_x_temp = v_x * cos((-imu) * DEG2RAD) - v_y * sin((-imu) * DEG2RAD);
        v_y_temp = v_x * sin((-imu) * DEG2RAD) + v_y * cos((-imu) * DEG2RAD);
        Current_time = ros::Time::now().toSec();
        fector = (Current_time - Begin_time)*2/3;
        if(fector>=1)
            fector = 1;
        v_x = v_x_temp*fector;
        v_y = v_y_temp*fector;
        if(fector >= 1){
            if(fabs(v_x) <= 0.05)                   // if x is near the target x of velocity will be zero
                v_x = 0;
            else if(fabs(v_y) <= 0.05)       // if y is near the target y of velocity will be zero
                v_y = 0;
            if (fabs(v_x) <= 0.05 && fabs(v_y) <= 0.05)
            {
                if (_CurrentTarget == 4)
                    _LocationState = finish;
                else
                {
                    _LocationState = reset_timer;
                    flag = TRUE;
                }
                _CurrentTarget++;
            }
        }
        break;
    case reset_timer:
        printf("reset timer\n");
        Begin_time = ros::Time::now().toSec();
        _LocationState = wait;
        break;
    case wait:
        printf("wait state\n");
        Current_time = ros::Time::now().toSec();
        v_x = 0;
        v_y = 0;
        if(Current_time - Begin_time >= 0.3){
            if(_Last_state == forward)
                _LocationState = back;
            else
                _LocationState = forward;
            Begin_time = ros::Time::now().toSec();
        }
        break;
    case finish: // Finish localization challange
        v_x = 0;
        v_y = 0;
        v_yaw = 0;
        break;
    case chase: // When robot lost the ball
        v_x = ball_dis * cos(ball_angle * DEG2RAD);
        v_y = ball_dis * sin(ball_angle * DEG2RAD);
        v_yaw = ball_angle;
        break;
    case error:
        printf("ERROR STATE\n");
        exit(FAULTEXECUTING);
        break;
    default: // ERROR SIGNAL
        printf("UNDEFINE STATE\n");
        exit(FAULTEXECUTING);
    }
    Normalization(v_yaw);
    if (imu > 1)
        IMU_state = 1;
    else if (imu < -1)
        IMU_state = 2;
    else if (fabs(imu) < 0.5)
        IMU_state = 0;
    else
        ; //do nothing
    switch (IMU_state)
    {
    case 0:
        v_yaw = 0;
        break;
    case 1:
        v_yaw = -imu;
        break;
    case 2:
        v_yaw = -imu;
        break;
    default:
        break;
    }
    _Env->Robot.v_x = v_x;
    _Env->Robot.v_y = v_y;
    _Env->Robot.v_yaw = v_yaw;
    if(_LocationState == forward || _LocationState == back)
        showInfo(Robot,imu);
}
void Strategy::Forward(RobotData &Robot, double &v_x, double &v_y, double &v_yaw, double imu, int &flag, double absolute_front, double compensation_x, double compensation_y)
{
    _Last_state = _LocationState;
    flag = FALSE;
    v_x = (_Target.TargetPoint[_CurrentTarget].x + compensation_x) - Robot.pos.x;
    v_y = (_Target.TargetPoint[_CurrentTarget].y + compensation_y) - Robot.pos.y;
    double v_x_temp, v_y_temp;
    //    <<<<<<<  HEAD   origin code in 2017.8.8
    // v_x_temp = v_x * cos((-imu) * DEG2RAD) - v_y * sin((-imu) * DEG2RAD);
    // v_y_temp = v_x * sin((-imu) * DEG2RAD) + v_y * cos((-imu) * DEG2RAD);
    //    >>>>>>>>  END   origin code in 2017.8.8

    //    <<<<<<<  HEAD   temp code in 2017.8.8
    double min_compensation = 0;                                            // because of yaw i need to rotate a min
    if((-imu) * DEG2RAD > 0)
    min_compensation = -3;                                                  // 3 is a value to compensation the yaw speed 
    else if ((-imu) * DEG2RAD < 0)
        min_compensation = 3;
    v_x_temp = v_x * cos((-imu + min_compensation) * DEG2RAD ) - v_y * sin((-imu + min_compensation) * DEG2RAD);       
    v_y_temp = v_x * sin((-imu + min_compensation) * DEG2RAD ) + v_y * cos((-imu + min_compensation) * DEG2RAD);
    //    >>>>>>>>  END   temp code in 2017.8.8
    v_x = v_x_temp;
    v_y = v_y_temp;
    v_yaw = atan2(_Target.TargetPoint[_CurrentTarget].y + compensation_y - Robot.pos.y, _Target.TargetPoint[_CurrentTarget].x + compensation_x - Robot.pos.x) * RAD2DEG - absolute_front;
    Normalization(v_yaw);
    if (fabs(v_yaw) < 3)
        v_yaw = 0;
    if (fabs(v_x) <= 0.1 && fabs(v_y) <= 0.1)
    {
        if (_CurrentTarget == _Target.size)
            _LocationState = finish;
        else
        {
            _LocationState = turn;
            _CurrentTarget++;
            flag = TRUE;
        }
    }
}
void Strategy::Turn(RobotData &Robot, double &v_x, double &v_y, double &v_yaw, double imu, int &flag, double absolute_front)
{
    static ros::Time last_time = ros::Time::now();
    ros::Time current_time = ros::Time::now();
    Vector3D vector_tr;
    vector_tr.x = _Target.TargetPoint[_CurrentTarget].x - Robot.pos.x;
    vector_tr.y = _Target.TargetPoint[_CurrentTarget].y - Robot.pos.y;
    vector_tr.yaw = atan2(vector_tr.y, vector_tr.x) * RAD2DEG - absolute_front;
    if (fabs(vector_tr.x) < 0.3 && fabs(vector_tr.y) < 0.3)
    {
        _CurrentTarget++;
        if (_CurrentTarget > _Target.size)
        {
            printf("error\n");
            _LocationState = finish;
        }
    }
    //    <<<<<<<  HEAD   origin code in 2017.8.8
    // v_y = 0.7;   // old value is 0.7 
    // v_x = 0;
    // v_yaw = vector_tr.yaw; // turn to target
    // Normalization(v_yaw);
    // if (fabs(v_yaw) <= 35)
    // {
    //     printf("Special slow motion \n");
    //     v_x = 0;
    //     v_y = 0;
    //     v_yaw = vector_tr.yaw; // turn to target
    //     if (fabs(v_yaw) <= 3)
    //     {
    //         printf("Change mode to forward \n");
    //         _LocationState = forward;
    //     }
    // }
    //    <<<<<<<  END    origin code in 2017.8.8

    //    <<<<<<<  HEAD   temp code in 2017.8.8
    double v_strike = 1.0;
    v_y = fabs(vector_tr.yaw/180.0)*v_strike;   
    v_x = 0;
    v_yaw = vector_tr.yaw; // turn to target
    Normalization(v_yaw);
    if (fabs(v_yaw) <= 5)
    {
        printf("Change mode to forward \n");
        _LocationState = forward;
    }
    //    <<<<<<<  END    temp code in 2017.8.8
}
void Strategy::Chase()
{
    ;
}
int Strategy::ThroughPath(int i, int j)
{
    double Slope = (_Location->LocationPoint[i].y - _Location->LocationPoint[j].y) / (_Location->LocationPoint[i].x - _Location->LocationPoint[j].x);
    if (Slope > 999)
        Slope = 999;
    double dis = (_Location->LocationPoint[j].y - Slope * _Location->LocationPoint[j].x) / sqrt(Slope * Slope + 1);
    if (fabs(dis) < 0.1)
        return TRUE;
    else
        return FALSE;
}
std::vector<int> Strategy::OptimatePath()
{
    for (int i = 0; i < 10; i++)
    {
        _Target.TargetPoint[i].x = 0;
        _Target.TargetPoint[i].y = 0;
        _Target.TargetPoint[i].angle = 0;
        _Target.size = 0;
    }
    std::vector<int> order;
    std::vector<int> enable_point;
    for (int i = 0; i < 5; i++)
        enable_point.push_back(i);
    std::vector<int>::iterator it;
    printf("\n");
    int order_counter = 0;
    int horizon_point = -1;
    int hotizon_location = -1;
    int temp = -1;
    double front = 0;
    double min_rotation = 999;
    double normalization_temp_angle;
    for (int i = 0; i < 5; i++)
    {
        if (_Location->LocationPoint[i].y == 0 || _Location->LocationPoint[i].x == 0)
        {
            horizon_point = i;
            if (_Location->LocationPoint[i].y > 0)
                hotizon_location = up;
            else if (_Location->LocationPoint[i].y < 0)
                hotizon_location = down;
            else if (_Location->LocationPoint[i].x > 0)
                hotizon_location = right;
            else if (_Location->LocationPoint[i].x < 0)
                hotizon_location = left;
        }
    }
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            if (ThroughPath(i, j))
                through_path_ary[i][j] = j + 1;
        }
    }
    switch (hotizon_location)
    {
    case up:
        // printf("up\n");
        // pick first point
        front = 90;
        order.push_back(horizon_point);
        EraseElement(enable_point, horizon_point);
        // pick second point
        front = -90;
        MinAngle(order, enable_point, front);
        // pick third point
        front = (_Location->LocationPoint[order.back()].angle + 180);
        Normalization(front);
        MinAngle(order, enable_point, front);
        // pick fourth point
        front = (_Location->LocationPoint[order.back()].angle + 180);
        Normalization(front);
        MinAngle(order, enable_point, front);
        // pick fifth point
        front = (_Location->LocationPoint[order.back()].angle + 180);
        Normalization(front);
        MinAngle(order, enable_point, front);
        order.push_back(-1);
        break;
    case down:
        // printf("down\n");
        // pick first point
        front = -90;
        order.push_back(horizon_point);
        EraseElement(enable_point, horizon_point);
        // pick second point
        front = 90;
        MinAngle(order, enable_point, front);
        // pick third point
        front = (_Location->LocationPoint[order.back()].angle + 180);
        Normalization(front);
        MinAngle(order, enable_point, front);
        // pick fourth point
        front = (_Location->LocationPoint[order.back()].angle + 180);
        Normalization(front);
        MinAngle(order, enable_point, front);
        // pick fifth point
        front = (_Location->LocationPoint[order.back()].angle + 180);
        Normalization(front);
        MinAngle(order, enable_point, front);
        order.push_back(-1);
        break;
    case right:
        // pick first point
        for (int i = 0; i < enable_point.size(); i++)
        {
            if (_Location->LocationPoint[enable_point[i]].angle > 0 && _Location->LocationPoint[enable_point[i]].angle < 90)
                temp = enable_point[i];
        }
        order.push_back(temp);
        EraseElement(enable_point, temp);
        // pick second point            probleming!!!!!!!!!!
        front = (_Location->LocationPoint[order.back()].angle + 180);
        Normalization(front);
        MinAngle(order, enable_point, front);
        // pick third point
        front = (_Location->LocationPoint[order.back()].angle + 180);
        Normalization(front);
        MinAngle(order, enable_point, front);
        // pick fourth point
        front = (_Location->LocationPoint[order.back()].angle + 180);
        Normalization(front);
        MinAngle(order, enable_point, front);
        // pick fifth point
        front = (_Location->LocationPoint[order.back()].angle + 180);
        Normalization(front);
        MinAngle(order, enable_point, front);
        order.push_back(-1);
        break;
    case left:
        // pick first point
        for (int i = 0; i < enable_point.size(); i++)
        {
            if (_Location->LocationPoint[enable_point[i]].angle > 90 && _Location->LocationPoint[enable_point[i]].angle < 180)
                temp = enable_point[i];
        }
        order.push_back(temp);
        EraseElement(enable_point, temp);
        // pick second point            probleming!!!!!!!!!!
        front = (_Location->LocationPoint[order.back()].angle + 180);
        Normalization(front);
        MinAngle(order, enable_point, front);
        // pick third point
        front = (_Location->LocationPoint[order.back()].angle + 180);
        Normalization(front);
        MinAngle(order, enable_point, front);
        // pick fourth point
        front = (_Location->LocationPoint[order.back()].angle + 180);
        Normalization(front);
        MinAngle(order, enable_point, front);
        // pick fifth point
        front = (_Location->LocationPoint[order.back()].angle + 180);
        Normalization(front);
        MinAngle(order, enable_point, front);
        order.push_back(-1);
        break;
    default:
        printf("UNDEFINE STATE\n");
        exit(FAULTEXECUTING);
    }
    _Target.size = order.size();
    for (int i = 0; i < order.size(); i++)
    {
        if (order[i] == -1)
        {
            _Target.TargetPoint[i].x = 0;
            _Target.TargetPoint[i].y = 0;
            _Target.TargetPoint[i].angle = 0;
        }
        else
        {
            _Target.TargetPoint[i].x = _Location->LocationPoint[order[i]].x;
            _Target.TargetPoint[i].y = _Location->LocationPoint[order[i]].y;
            _Target.TargetPoint[i].angle = _Location->LocationPoint[order[i]].angle;
        }
    }
    // =================   printf final point to run  ============================
    //     for(int i=0;i<_Target.size;i++)
    //     {
    //         printf("%d : x=%lf\ty=%lf\tangle=%lf\n",i,_Target.TargetPoint[i].x,_Target.TargetPoint[i].y,_Target.TargetPoint[i].angle);
    //     }
    // =================   printf order  and   unrunning point with __ to determind   ===================
    // for (int i = 0; i < order.size(); i++)          // -1 is origin point
    // {
    //     printf("%d\t", order[i]);
    // }
    // printf("\n");
    // for (int i = 0; i < enable_point.size(); i++)
    //     printf("%d__", enable_point[i]);
    // printf("\n");
    return order;
}
void Strategy::showInfo(std::vector<int> order, double imu, double compensation_x, double compensation_y)
{
    std::string Sv_x = "→";
    std::string Sv_y = "↑";
    std::string Sv_yaw = "↶";
    if (_Env->Robot.v_x > 0.1)
        Sv_x = "→ ";
    else if (_Env->Robot.v_x < -0.1)
        Sv_x = "← ";
    else
        Sv_x = "";
    if (_Env->Robot.v_y > 0.1)
        Sv_y = "↑ ";
    else if (_Env->Robot.v_y < -0.1)
        Sv_y = "↓ ";
    else
        Sv_y = "";
    if (_Env->Robot.v_yaw > 0.001)
        Sv_yaw = "↶";
    else if (_Env->Robot.v_yaw < -0.001)
        Sv_yaw = "↷";
    else
        Sv_yaw = "";
    printf("***********************************************\n");
    printf("*                  START                      *\n");
    printf("***********************************************\n");

    //   ====================   test   =========================
    switch (_LocationState)
    {
    case forward:
        printf("Target : Point %d\t", order[_CurrentTarget] + 1);
        printf("State : Forward\n");
        break;
    case finish:
        printf("State : Fisish\n");
        break;
    case chase:
        printf("Target : Ball\t");
        printf("State : Chase\n");
        break;
    case turn:
        printf("Target : Point %d\t", order[_CurrentTarget] + 1);
        printf("State : Turn\n");
        break;
    case error:
        printf("State : Error\n");
        break;
    }

    if (_LocationState == forward)
        std::cout << "Target position : (" << _Target.TargetPoint[_CurrentTarget].x + compensation_x
                  << "," << _Target.TargetPoint[_CurrentTarget].y + compensation_y << ")\n";
    else if (_LocationState == chase)
        std::cout << "Target position : (" << _Env->Robot.pos.x + _Env->Robot.ball.distance * cos((_Env->Robot.pos.angle + _Env->Robot.ball.angle + 90) * DEG2RAD)
                  << "," << _Env->Robot.pos.y + _Env->Robot.ball.distance * sin((_Env->Robot.pos.angle + _Env->Robot.ball.angle + 90) * DEG2RAD)
                  << ")" << std::endl;
    else if (_LocationState == turn)
        if (_Last_state == forward)
            std::cout << "Target position : (" << _Target.TargetPoint[_CurrentTarget].x + compensation_x
                      << "," << _Target.TargetPoint[_CurrentTarget].y + compensation_y << ")\n";
        else
            std::cout << "Target position : (" << _Target.TargetPoint[_CurrentTarget].x + compensation_x
                      << "," << _Target.TargetPoint[_CurrentTarget].y + compensation_y << ")\n";

    std::cout << "Imu = " << imu << std::endl;
    std::cout << "Robot position : (" << _Env->Robot.pos.x << "," << _Env->Robot.pos.y << ")\n";
    std::string haha = Sv_x + Sv_y + Sv_yaw;
    std::cout << "Direction : " << Sv_x + Sv_y + Sv_yaw << std::endl;
    printf("Speed : (%3f,%3f,%3f)\n", _Env->Robot.v_x, _Env->Robot.v_y, _Env->Robot.v_yaw);
    printf("==================== END ======================\n\n");
}
void Strategy::showInfo(RobotData Robot,double imu)
{
    double ball_dis = 0.28;          // if you don't want to get information by vision
    double ball_angle = 0.0;          // if you don't want to get information by vision
    // double ball_dis = _Env->Robot.ball.distance;     //get ball information by vision
    // double ball_angle = _Env->Robot.ball.angle;      //get ball information by vision
    std::string Sv_x = "→";
    std::string Sv_y = "↑";
    std::string Sv_yaw = "↶";
    if (_Env->Robot.v_x > 0.1)
        Sv_x = "→ ";
    else if (_Env->Robot.v_x < -0.1)
        Sv_x = "← ";
    else
        Sv_x = "";
    if (_Env->Robot.v_y > 0.1)
        Sv_y = "↑ ";
    else if (_Env->Robot.v_y < -0.1)
        Sv_y = "↓ ";
    else
        Sv_y = "";
    if (_Env->Robot.v_yaw > 0.001)
        Sv_yaw = "↶";
    else if (_Env->Robot.v_yaw < -0.001)
        Sv_yaw = "↷";
    else
        Sv_yaw = "";
    printf("***********************************************\n");
    printf("*                  START                      *\n");
    printf("***********************************************\n");

    switch (_LocationState)
    {
    case forward:
        printf("Target : Point %d\t", _CurrentTarget);
        printf("State : Forward\n");
        break;
    case back:
        printf("Target : Point %d\t", _CurrentTarget);
        printf("State : Back\n");
        break;
    case finish:
        printf("State : Fisish\n");
        break;
    case chase:
        printf("Target : Ball\t");
        printf("State : Chase\n");
        break;
    case turn:
        printf("Target : Point %d\t", _CurrentTarget);
        printf("State : Turn\n");
        break;
    case error:
        printf("State : Error\n");
        break;
    }
    if (_LocationState == forward)
        std::cout << "Target position : (" << _Location->LocationPoint[_CurrentTarget].x
                  << "," << _Location->LocationPoint[_CurrentTarget].y << ")\n";
    else if (_LocationState == back)
        std::cout << "Target position : (" << _Location->MiddlePoint[_CurrentTarget].x
                  << "," << _Location->MiddlePoint[_CurrentTarget].y << ")\n";
    else if (_LocationState == chase)
        std::cout << "Target position : (" << _Env->Robot.pos.x + ball_dis * cos((_Env->Robot.pos.angle + ball_angle + 90) * DEG2RAD)
                  << "," << _Env->Robot.pos.y + ball_dis * sin((_Env->Robot.pos.angle + ball_angle + 90) * DEG2RAD)
                  << ")" << std::endl;
    else if (_LocationState == turn)
        if (_Last_state == forward)
            std::cout << "Target position : (" << _Location->MiddlePoint[_CurrentTarget].x
                      << "," << _Location->MiddlePoint[_CurrentTarget].y << ")\n";
        else
            std::cout << "Target position : (" << _Location->MiddlePoint[_CurrentTarget].x
                      << "," << _Location->MiddlePoint[_CurrentTarget].y << ")\n";
    std::cout << "Imu = " << imu << std::endl;
    std::cout << "Ball position : (" << Robot.ball.x << "," << Robot.ball.y << ")\n";
    std::string haha = Sv_x + Sv_y + Sv_yaw;
    std::cout << "Direction : " << Sv_x + Sv_y + Sv_yaw << std::endl;
    printf("Speed : (%3f,%3f,%3f)\n", _Env->Robot.v_x, _Env->Robot.v_y, _Env->Robot.v_yaw);
    printf("==================== END ======================\n\n");
}
void Strategy::Normalization(double &angle)
{
    if (angle > 180)
        angle -= 360;
    else if (angle < -180)
        angle += 360;
}
void Strategy::EraseElement(std::vector<int> &vec, int index)
{
    for (int i = 0; i < vec.size(); i++)
    {
        if (vec[i] == index)
        {
            index = i;
            break;
        }
    }
    std::vector<int>::iterator it = vec.begin() + index;
    vec.erase(it);
}
void Strategy::MinAngle(std::vector<int> &order, std::vector<int> &enable_point, int front)
{
    int back_flag = 1;
    int temp;
    double min_rotation = 999;
    double normalization_temp_angle;
    for (int i = 0; i < enable_point.size(); i++)
    {
        normalization_temp_angle = fabs(_Location->LocationPoint[enable_point[i]].angle - front);
        Normalization(normalization_temp_angle);
        if (fabs(normalization_temp_angle) < min_rotation)
        {
            min_rotation = fabs(normalization_temp_angle);
            temp = enable_point[i];
        }
        if (through_path_ary[order.back()][enable_point[i]])
        {
            temp = enable_point[i];
            back_flag = 0;
        }
    }
    if (back_flag)
        order.push_back(-1);
    order.push_back(temp);
    EraseElement(enable_point, temp);
}
