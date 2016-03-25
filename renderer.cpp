#include <string>

#include "opencv2/opencv.hpp"
#include "renderer.hpp"

GloveRenderer::GloveRenderer() {

}

GloveRenderer::~GloveRenderer() {

}

//temp assuming already setup

FullHandPose GloveRenderer::LoadFullHandPose(string handpose_yml) {
    try {
        // Now we're going to change the hand pose and render again
        // The hand pose depends on the number of bones, which is specified
        // by the scene spec file.
        hand_pose = FullHandPose(scene_spec.num_bones());
        hand_pose.Load(handpose_yml, scene_spec);
        return hand_pose;
    } catch (const std::exception &e) {
        cerr << "Exception: " << e.what() << endl;
    }

}

void GloveRenderer::Setup(string scene_spec_filename) {
    // Make sure to always catch exceptions around the LibHand code.
    // LibHand uses a "RAII" pattern to provide for a clean shutdown in
    // case of any errors.
    try {
        glove_renderer.Setup();

        // Process the scene spec file
        scene_spec = SceneSpec(scene_spec_filename);


        // Tell the renderer to load the scene
        glove_renderer.LoadScene(scene_spec);

        // Now we're going to change the hand pose and render again
        // The hand pose depends on the number of bones, which is specified
        // by the scene spec file.
        hand_pose = FullHandPose(scene_spec.num_bones());
        hand_pose.Load("../../libhand/poses/relaxed2.yml", scene_spec);

        camera_spec = HandCameraSpec(glove_renderer.initial_cam_distance());
        glove_renderer.set_camera_spec(camera_spec);
        hand_pose.SetRotMatrix(camera_spec);
        glove_renderer.SetHandPose(hand_pose);

        // Now we render a hand using a default pose
        glove_renderer.RenderHand();
    } catch (const std::exception &e) {
        cerr << "Exception: " << e.what() << endl;
    }
}

cv::Mat GloveRenderer::Render(FullHandPose hand_pose, HandCameraSpec camera_spec) {
    // Make sure to always catch exceptions around the LibHand code.
    // LibHand uses a "RAII" pattern to provide for a clean shutdown in
    // case of any errors.
    try {
        glove_renderer.set_camera_spec(camera_spec);
        hand_pose.SetRotMatrix(camera_spec);
        glove_renderer.SetHandPose(hand_pose);

        // Then we will render the hand again and show it to the user.
        glove_renderer.RenderHand();
        //cv::imwrite(concatString("searchSet/", index, ".png"),pic,param);
    } catch (const std::exception &e) {
        cerr << "Exception: " << e.what() << endl;
    }

    return glove_renderer.pixel_buffer_cv();
}

HandCameraSpec GloveRenderer::GetHandCameraSpec() {
    return camera_spec;
}

string concatString(string a, int b, string c) {
    std::stringstream ss;
    ss << a;
    ss << b;
    ss << c;
    return ss.str();
}

SceneSpec GloveRenderer::GetSceneSpec() {
    return scene_spec;
}

FullHandPose GloveRenderer::GetFullHandPose() {
    return hand_pose;
}