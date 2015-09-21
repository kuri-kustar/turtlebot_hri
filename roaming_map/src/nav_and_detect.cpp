
#include <ros/ros.h>
#include <move_base_msgs/MoveBaseAction.h>
#include <actionlib/client/simple_action_client.h>
#include "face_recognition/DAF.h"
#include <geometry_msgs/Twist.h>

typedef actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> MoveClient;

double MAP_X_COORDINATES[4] = {3.01,4.59,0.58,-1.49};
double MAP_Y_COORDINATES[4] = {0.04,6.98,8.2,0.93};
#define X MAP_X_COORDINATES
#define Y MAP_Y_COORDINATES
#define RESET_VALUE 1000

class NavNodeInit
{
public:
   NavNodeInit(): mc("move_base", true)
   {
      while (!mc.waitForServer(ros::Duration(0.0))) ROS_INFO("Waiting for server");
      //subscribes to the "whatNext" topic, on which the face_recognition client publishes "MOVE" or "STOP", depending on face detection results
      sub = nh.subscribe("whatNext", 1, &NavNodeInit::motionCallback, this);
      next_goal_id = 0;
      prev_goal_id = RESET_VALUE;
      curr_x = RESET_VALUE;
      curr_y = RESET_VALUE;
      W = 1; //arbitrary orientation
   }

   //move_base actionlib feedback funciton. stores current xy coordinates, as returned by move_base/feedback. current coordinates are used to preempt the execution of the current goal with the next goal when the turtlebot is close to the target_pose, to prevent the turtlebot from stopping at the end of each goal
   void feedbackCB(const move_base_msgs::MoveBaseFeedbackConstPtr& feedback)
   {
      curr_x = feedback->base_position.pose.position.x;
      curr_y = feedback->base_position.pose.position.y;
   }
   //subscriber callback function. it takes care of goal sending, and stopping when a face is detected.
   void motionCallback(const face_recognition::DAFConstPtr& msg)
   {    
      ROS_INFO("pgi = %d", prev_goal_id);
      ROS_INFO("ngi = %d", next_goal_id);
      ROS_INFO("%s", (mc.isServerConnected() ? "true" : "false"));

      if (msg->detected_a_face == "MOVE")
      {
	 if (next_goal_id != prev_goal_id)
         {
            prev_goal_id = next_goal_id;
            NextGoal.target_pose.header.frame_id = "map";
            NextGoal.target_pose.header.stamp = ros::Time::now();
            NextGoal.target_pose.pose.position.x = X[next_goal_id];
            NextGoal.target_pose.pose.position.y = Y[next_goal_id];
            NextGoal.target_pose.pose.orientation.x = 0;
            NextGoal.target_pose.pose.orientation.y = 0;
            NextGoal.target_pose.pose.orientation.z = 0;
            NextGoal.target_pose.pose.orientation.w = W;
            ROS_INFO("Sending new goal");
            mc.sendGoal(NextGoal, MoveClient::SimpleDoneCallback(), MoveClient::SimpleActiveCallback(), boost::bind(&NavNodeInit::feedbackCB, this, _1));

         }
         mc.waitForResult(ros::Duration(0.1));

         if ((abs(curr_x-NextGoal.target_pose.pose.position.x)<0.7) && (abs(curr_y-NextGoal.target_pose.pose.position.y)<0.7))
              next_goal_id = (next_goal_id==3) ? 0 : (next_goal_id + 1);

      }
      else if (msg->detected_a_face == "STOP" && prev_goal_id!=RESET_VALUE)
      {     
	 mc.cancelAllGoals();
         prev_goal_id = RESET_VALUE;
         ros::Duration(0.8).sleep();
      }
      else 
      {
         ros::Duration(0.4).sleep();
      }
   }

protected:
   MoveClient mc;
   ros::NodeHandle nh;
   ros::Subscriber sub;
   geometry_msgs::Twist nav_msg;
   move_base_msgs::MoveBaseGoal NextGoal;
   int next_goal_id; //IDs to iterate over the 4 room corners
   int prev_goal_id;
   double curr_x; //current x from move_base feedback
   double curr_y; //current y from move_base feedback
   double W; //orientation

};

int main(int argc, char** argv)
{
   ros::init(argc, argv, "nav_and_detect");
   NavNodeInit nav_node;

   ros::spin();

   return 0;
}