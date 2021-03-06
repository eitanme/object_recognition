/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2009, Willow Garage, Inc.
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
 *   * Neither the name of Willow Garage, Inc. nor the names of its
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
 *
 */

#include <vector>

#include <ecto/ecto.hpp>

#include <Eigen/Geometry>

// ROS includes
#include <ros/publisher.h>
#include <std_msgs/String.h>
#include <geometry_msgs/PoseArray.h>
#include <sensor_msgs/Image.h>

#include <opencv2/core/core.hpp>

typedef unsigned int ObjectId;

namespace object_recognition
{
  struct PoseArrayAssembler
  {
    typedef geometry_msgs::PoseArrayConstPtr output_t;

    void
    configure(const ecto::tendrils& params, const ecto::tendrils& inputs, const ecto::tendrils& outputs)
    {
    }

    static void
    declare_io(const ecto::tendrils& params, ecto::tendrils& inputs, ecto::tendrils& outputs)
    {
      inputs.declare<std::vector<ObjectId> >("object_ids", "the id's of the found objects");
      inputs.declare<sensor_msgs::ImageConstPtr>("image_message", "the image message to get the header");
      inputs.declare<std::vector<cv::Mat> >("Rs", "The rotations of the poses of the found objects");
      inputs.declare<std::vector<cv::Mat> >("Ts", "The translations of the poses of the found objects");
      outputs.declare<output_t>("pose_message", "The poses");
    }

    int
    process(const ecto::tendrils& inputs, const ecto::tendrils& outputs)
    {
      const std::vector<cv::Mat> & Rs = inputs.get<std::vector<cv::Mat> >("Rs");
      const std::vector<cv::Mat> & Ts = inputs.get<std::vector<cv::Mat> >("Ts");

      static geometry_msgs::PoseArray pose_array_msgs;
      static std::vector<geometry_msgs::Pose> &poses = pose_array_msgs.poses;
      poses.resize(Rs.size());

      unsigned int i;
      for (i = 0; i < Rs.size(); ++i)
      {
        cv::Mat_<float> T, R;
        Ts[i].convertTo(T, CV_32F);
        Rs[i].convertTo(R, CV_32F);

        geometry_msgs::Pose & msg_pose = poses[i];

        Eigen::Matrix3f rotation_matrix;
        for (unsigned int j = 0; j < 3; ++j)
          for (unsigned int i = 0; i < 3; ++i)
            rotation_matrix(j, i) = R(j, i);

        Eigen::Quaternion<float> quaternion(rotation_matrix);

        msg_pose.position.x = T(0);
        msg_pose.position.y = T(1);
        msg_pose.position.z = T(2);
        msg_pose.orientation.x = quaternion.x();
        msg_pose.orientation.y = quaternion.y();
        msg_pose.orientation.z = quaternion.z();
        msg_pose.orientation.w = quaternion.w();
      }

      // Publish the different poses
      ros::Time time = ros::Time::now();

      std::string frame_id = inputs.get<sensor_msgs::ImageConstPtr>("image_message")->header.frame_id;
      pose_array_msgs.header.stamp = time;
      pose_array_msgs.header.frame_id = frame_id;

      outputs["pose_message"] << output_t(new geometry_msgs::PoseArray(pose_array_msgs));

      return 0;
    }
  };
}

ECTO_CELL(ros, object_recognition::PoseArrayAssembler, "PoseArrayAssembler",
          "Given object ids and poses, publish ROS messages.");
