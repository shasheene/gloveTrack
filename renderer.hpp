#ifndef RENDERER_HPP
#define RENDERER_HPP

// We need the HandPose data structure
#include "include/libhand/hand_pose.h"

// ..the HandRenderer class which is used to render a hand
#include "include/libhand/hand_renderer.h"

// ..and SceneSpec which tells us where the hand 3D scene data
// is located on disk, and how the hand 3D object relates to our
// model of joints.
#include "include/libhand/scene_spec.h"

// Don't forget to mention the libhand namespace
using namespace libhand;
using namespace std;

class GloveRenderer {
public:
    GloveRenderer();
    virtual ~GloveRenderer();
    void Setup(string scene_spec_filename);
    cv::Mat Render(FullHandPose hand_renderer, HandCameraSpec camera_spec);
    HandCameraSpec GetHandCameraSpec();
    FullHandPose LoadFullHandPose(string handpose_yml);
    SceneSpec GetSceneSpec();
    FullHandPose GetFullHandPose();

private:
    string concatString(string a, int b, string c);
    // Setup the hand renderer
    HandRenderer glove_renderer;
    FullHandPose hand_pose;
    HandCameraSpec camera_spec;
    SceneSpec scene_spec;
};

#endif

