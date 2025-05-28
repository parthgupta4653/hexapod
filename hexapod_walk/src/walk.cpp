#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "bits/stdc++.h"
using namespace std;

vector<string> leg_names = {"l1", "l2", "l3", "r1", "r2", "r3"};
vector<string> joint_names = {"coxa", "femur", "tibia"};
class Hexapod_Walk_node : public rclcpp::Node {
private:
    void reset_joints() {
        for (int i = 0; i < 6; ++i) {
            for (int j = 0; j < 3; ++j) {
                std_msgs::msg::Float64 msg;
                msg.data = 0.0;
                joint_publishers[i][j]->publish(msg);
            }
        }
    }
    void move_timer_callback() {
        this->flag = (this->flag+1)%2;
        int rotating;
        double speed;

        if(this->linear_speed==0.0 && this->angular_speed==0.0){
            this->reset_joints();
            return;
        }
        else if(this->linear_speed!=0.0 && this->angular_speed==0.0){
            RCLCPP_INFO(this->get_logger(), "Moving straight with linear speed: %f", this->linear_speed);
            speed = this->linear_speed/4;
            rotating = 1;
        }
        else if(this->linear_speed==0.0 && this->angular_speed!=0.0){
            RCLCPP_INFO(this->get_logger(), "Rotating with angular speed: %f", this->angular_speed);
            speed = this->angular_speed/6;
            rotating = -1;
        }
        else{
            RCLCPP_INFO(this->get_logger(), "Invalid command received, stopping.");
            this->reset_joints();
            return;
        }

        for(int i=0; i<20; i++){
            double phase = i/20.0;            
            vector<double> angles_stance = {phase*speed,0,0};
            vector<double> angles_swing = {phase*speed, -0.4*sin(phase*M_PI), 0.21*sin(phase*M_PI)};

            //RCLCPP_INFO(this->get_logger(), "stance: t1: %f, t2: %f, t3: %f", angles_stance[0], angles_stance[1], angles_stance[2]);
            //RCLCPP_INFO(this->get_logger(), "swing: t1: %f, t2: %f, t3: %f", angles_swing[0], angles_swing[1], angles_swing[2]);

            for(int j=0; j<6; j++){
                std_msgs::msg::Float64 msg1, msg2, msg3;
                if(j%2==this->flag){
                    //RCLCPP_INFO(this->get_logger(), "Stance phase for leg %d", j);
                    if(j>2)
                        msg1.data = rotating * -angles_stance[0];
                    else
                        msg1.data = angles_stance[0];
                    msg2.data = angles_stance[1];
                    msg3.data = angles_stance[2];
                }else{
                    //RCLCPP_INFO(this->get_logger(), "Swing phase for leg %d", j);
                    if(j>2) 
                        msg1.data = rotating * angles_swing[0];
                    else
                        msg1.data = -angles_swing[0];
                    msg2.data = angles_swing[1];
                    msg3.data = angles_swing[2];
                }
                joint_publishers[j][0]->publish(msg1);
                joint_publishers[j][1]->publish(msg2);
                joint_publishers[j][2]->publish(msg3);
            }
        }
    }
public:
    vector<vector<rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr>> joint_publishers; // 6 legs, 3 joints each
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_subscriber;
    rclcpp::TimerBase::SharedPtr timer;
    int flag = 1;

    double linear_speed = 0.0;
    double angular_speed = 0.0;
    
    Hexapod_Walk_node() : Node("Hexapod_Walking_node") {
        RCLCPP_INFO(this->get_logger(), "Walker node has been created.");

        for (int i = 0; i < 6; ++i) {
            vector<rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr> leg_publishers;
            for (int j = 0; j < 3; ++j) {
                auto publisher = this->create_publisher<std_msgs::msg::Float64>("/joint/" + joint_names[j] + "_joint_" + leg_names[i] + "/cmd_pos", 10);
                leg_publishers.push_back(publisher);
            }
            joint_publishers.push_back(leg_publishers);
        }
        
        cmd_vel_subscriber = this->create_subscription<geometry_msgs::msg::Twist>(
            "/cmd_vel", 10, 
            [this](const geometry_msgs::msg::Twist::SharedPtr msg) {
                this->linear_speed = msg->linear.x;
                this->angular_speed = -msg->angular.z;
                RCLCPP_INFO(this->get_logger(), "Received cmd_vel: linear: %f, angular: %f", this->linear_speed, this->angular_speed);
            });
        this->move(); // Call the function to move straight
    }

    void move() {       
        // Send commands to all joints to reset them
        this->reset_joints();

        RCLCPP_INFO(this->get_logger(), "Straight movement command sent.");
        this_thread::sleep_for(chrono::milliseconds(2000)); // Wait for the period duration
        //this->move_timer_callback();
        timer = this->create_wall_timer(chrono::milliseconds(1000), bind(&Hexapod_Walk_node::move_timer_callback, this));
    }

};

int main(){
    rclcpp::init(0, nullptr);
    auto node = std::make_shared<Hexapod_Walk_node>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
