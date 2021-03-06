/*

Copyright 2015.

This file is part of the face_dr ROS package.

face_dr is free software: you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation, version 3.

face_dr is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
Lesser GNU General Public License for more details.

You should have received a copy of the Lesser GNU General Public License
along with face_dr.  If not, see <http://www.gnu.org/licenses/>.


Author: Alaa El Khatib
Last updated: 20.12.2015

The code below is inspired by, builds upon, and/or uses code from the following source(s):

-face_recognition ROS package by Pouyan Ziafati, shared under a Creative Commons Attribution-NonCommercial 3.0 Unported license (http://creativecommons.org/licenses/by-nc/3.0/).

-OpenCV's FaceReconizer tutorial (http://docs.opencv.org/modules/contrib/doc/facerec/facerec_tutorial.html) code by Philipp Wagner, shared under a BSD Simplified license (http://www.opensource.org/licenses/bsd-license). The license terms are reproduced below.
[*
* Copyright (c) 2011. Philipp Wagner <bytefish[at]gmx[dot]de>.
* Released to public domain under terms of the BSD Simplified license.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the
*     documentation and/or other materials provided with the distribution.
*   * Neither the name of the organization nor the names of its contributors
*     may be used to endorse or promote products derived from this software
*     without specific prior written permission.
*]

-OpenCV's Histogram Calculation tutorial (http://docs.opencv.org/doc/tutorials/imgproc/histograms/histogram_calculation/histogram_calculation.html).

*/


#ifndef FDR_SERVER_H
#define FDR_SERVER_H

#include "ros/ros.h"
#include "actionlib/server/simple_action_server.h"
#include "image_transport/image_transport.h"
#include "image_transport/subscriber_filter.h"
#include "message_filters/synchronizer.h"
#include "message_filters/sync_policies/approximate_time.h"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "face_dr/fdrAction.h"
#include "sensor_msgs/image_encodings.h"
#include "opencv2/ml/ml.hpp"
#include "cv_bridge/cv_bridge.h"
#include <tf/transform_listener.h>
#include <tf/transform_broadcaster.h>

typedef actionlib::SimpleActionServer<face_dr::fdrAction> FDR_SERVER;
typedef face_dr::fdrFeedback FDR_FEEDBACK;
typedef face_dr::fdrResult FDR_RESULT;
typedef face_dr::fdrGoalConstPtr FDR_GOAL_Ptr;

//Detection & recognition need synchronized RGB and depth_registered images
typedef message_filters::sync_policies::ApproximateTime<sensor_msgs::Image, sensor_msgs::Image> img_sync_policy;

/*
Define paths to folders and files.
Make sure to have the following file hierarchy EXACTLY:

..
└── ros_data
├── faces
│   ├── face_images
│   ├── haarcascade_frontalface_alt.xml
│   └── lbpcascade_frontalface.xml
└── upper_body
├── dc_images
├── haarcascade_upperbody.xml
└── hue_hists

If you delete any training data from the folders, restore to the above.
DO NOT delete individual training images; delete the entire folder that is generated for each person.
If you delete stuff, re-train.
*/

const std::string ROS_DATA = "/ros_data/"; //THIS IS THE ONLY FILE PATH YOU SHOULD CHANGE!
const std::string FACE_FOLDER = ROS_DATA + "faces/";
const std::string FACE_IMGS_FOLDER = FACE_FOLDER + "face_images/";
const std::string U_BODY_FOLDER = ROS_DATA + "upper_body/";
const std::string HUE_HISTS_FOLDER = U_BODY_FOLDER + "hue_hists/";
const std::string DRESS_COLOR_IMGS_FOLDER = U_BODY_FOLDER + "dc_images/";


const std::string FACE_CSV = FACE_FOLDER + "faces_csv.txt";
const std::string FACE_HAAR_DETECTOR = FACE_FOLDER + "haarcascade_frontalface_alt.xml";
const std::string FACE_LBP_DETECTOR = FACE_FOLDER + "lbpcascade_frontalface.xml";
const std::string FACE_PCA_RECOGNIZER = FACE_FOLDER + "pca_model.yml";
const std::string FACE_NAMES = FACE_FOLDER + "class_names.txt";

const std::string U_BODY_HAAR_DETECTOR = U_BODY_FOLDER + "haarcascade_upperbody.xml";
const std::string COLOR_CLASSIFIER = U_BODY_FOLDER + "color_classifier.yml";
const std::string U_BODY_NAMES = U_BODY_FOLDER + "class_names.txt";

//camera parameters used to estimate distances
const double focal_length = 531;
const double frame_width = 640;
const double frame_height = 480;

//define face recognizer parameters
const int PCA_DIM = 30;
const int PCA_THRSH = 8000; //TUNE THRESHOLD ACCORDING TO YOUR SET!

class FDRServer
{

	ros::NodeHandle nh;
	FDR_SERVER fdr_as;

	image_transport::ImageTransport img_transp;
	image_transport::SubscriberFilter rgb_sub;
	image_transport::SubscriberFilter depth_sub;
	message_filters::Synchronizer<img_sync_policy> sync;

	FDR_FEEDBACK fdr_feedback;
	FDR_RESULT fdr_result;
	FDR_GOAL_Ptr fdr_goal;
	cv::CascadeClassifier face_cascade_1; //loads Haar
	cv::CascadeClassifier face_cascade_2; //laods LBP
	cv::CascadeClassifier face_cascade; //uses either Haar or LBP depending on order
	cv::CascadeClassifier ub_cascade; //loads Haar

	//transform from turtlebot cam frame to map
	tf::TransformListener listener;

	//transform from standalone cam frame to map (SHOULD BE SET MANUALLY IN the .cpp FILE!)
	tf::Transform transform_sa;


	bool STANDALONE;



public:

	/*
	a server object is constructed with 3 arguments:
	-the name of the node
	-the name of the camera to use
	-a third string indicating wether the client is run on a turtlebot, or on a standalone (fixed) camera:
	---if the third string = "true", the client is run in standalone mode, otherwise in turtlebot.
	The last 2 arguments are passed as commnad-line args when you rosrun the node (the first is passed by in code in fdr_server_node.cpp)
	*/

	FDRServer(std::string, std::string, std::string);
	void goalCB();
	void preemptCB();
	void face_cb(const sensor_msgs::ImageConstPtr& rgb_img_ptr, const sensor_msgs::ImageConstPtr& depth_img_ptr);

};

#endif
