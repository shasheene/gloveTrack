#include <string>
#include<iostream>
#include <iomanip>
#include<ctime>
#include<cmath>


#include "opencv2/opencv.hpp"
#include "renderer.hpp"
#include "generatedb.hpp"

GenerateDb::GenerateDb() {

}

GenerateDb::~GenerateDb() {

}

void GenerateDb::Setup(GloveRenderer* renderer, string target_directory2) {
    glove_renderer = renderer;
    target_directory = target_directory2;
}

void GenerateDb::interpolate(int num_camera_angles, int num_poses_per_camera_angle, FullHandPose min_pose, FullHandPose max_pose, Manifest &manifest, GloveTrack glove_track) {
    auto console = spdlog::get("console");
    HandCameraSpec initial_camera_spec = glove_renderer->GetHandCameraSpec();
    initial_camera_spec.theta = 0;
    initial_camera_spec.phi = 3.1415;
    initial_camera_spec.tilt = 3.1415 / 2;
    HandCameraSpec modified_camera_spec = initial_camera_spec;


    FullHandPose incremental_pose = CalculateIncrementalDifference(num_poses_per_camera_angle, min_pose, max_pose);
    spdlog::get("console")->error("Incremental pose is b{}, t{}, s{}", incremental_pose.bend(0), incremental_pose.twist(0), incremental_pose.side(0));

    // We have 3 axises to modify and 
    int num_images = num_camera_angles; // 3;

    //camera_spec.r += 0.01; //hand_renderer.initial_cam_distance();

    for (int i = 0; i < num_poses_per_camera_angle; i++) {
        min_pose = IncrementPose(min_pose, incremental_pose);

        for (int j = 0; j <= num_camera_angles; j++) {
            // Get images from wide variety of camera angles
            //if ((j % 3) == 0) {
            //modified_camera_spec.theta += 2 * (3.1415) / num_images;
            //} else if ((j % 3) == 1) {
            modified_camera_spec.phi += 2 * (3.1415) / num_images;
            //} else if ((j % 3) == 2) {
            //modified_camera_spec.tilt += 2 * (3.1415) / num_images;
            //}
            
            //TODO(shasheene@gmail.com): Fix exception handling here
            try {
                Mat pic = glove_renderer->Render(min_pose, modified_camera_spec);
                //manifest.unnormalized_images.push_back(pic.clone());
                cv::imshow("Generated hand pose database entry", pic);
                
                SPDLOG_TRACE(console, "Running EM on query image");
                glove_track.GetHandPose(pic);
                Mat labelled = glove_track.GetLastNormalizedImage();
                manifest.labelled_images.push_back(labelled.clone());
                imshow("Labelled images of generated poses (not fully calibrated model)", labelled);

                waitKey(4);
            } catch (...) {
                SPDLOG_TRACE(console, "OpenCV displaying image of size 0");
            }


            // Needs this to render
            waitKey(4);
        }
    }
}

string GenerateDb::GenerateFilename(string target_directory, int index, int padding_zeroes) {
    // backup formatting standard
    ios init(NULL);
    init.copyfmt(cout);

    std::stringstream index_ss;
    index_ss << std::setfill('0') << std::setw(padding_zeroes);
    index_ss << index;

    //restore formatting standard
    cout.copyfmt(init);

    std::stringstream path_ss;
    path_ss << target_directory << "/";
    path_ss << "searchSet_" << index_ss.str() << ".png";
    return path_ss.str();

}

string GenerateDb::GenerateTimestamp() {
    // Quick non-C++ function to generate date stamp, from SO thread
    time_t current_time;
    struct tm* s_localtime;
    char buffer[100];
    time(&current_time);
    s_localtime = localtime(&current_time);

    strftime(buffer, 100, "%Y%m%d_%I%M%S", s_localtime);
    string to_return(buffer);

    return to_return;

}

FullHandPose GenerateDb::CalculateIncrementalDifference(float num_poses, FullHandPose min_pose, FullHandPose max_pose) {
    // temp hard coded num joints
    for (int i = 0; i < 18; i++) {
        float bend_increment = (max_pose.bend(i) - min_pose.bend(i)) / num_poses;
        float side_increment = (max_pose.side(i) - min_pose.side(i)) / num_poses;
        float twist_increment = (max_pose.twist(i) - min_pose.twist(i)) / num_poses;

        HandJoint hand_joint = HandJoint(bend_increment, side_increment, twist_increment);
        max_pose.set_joint(i, hand_joint);
    }
    return max_pose;
}

FullHandPose GenerateDb::IncrementPose(FullHandPose min_pose, FullHandPose increment) {
    // temp hard coded num joints
    for (int i = 0; i < 18; i++) {
        min_pose.bend(i) += increment.bend(i);
        min_pose.side(i) += increment.side(i);
        min_pose.twist(i) += increment.twist(i);
    }
    return min_pose;
}
