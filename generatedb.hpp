#ifndef GENERATEDB_HPP
#define GENERATEDB_HPP

#include "renderer.hpp"
#include "manifest.hpp"
#include "isolateGlove.hpp"

// Don't forget to mention the libhand namespace
using namespace libhand;
using namespace std;

class GenerateDb {
public:
    GenerateDb();
    virtual ~GenerateDb();
    void Setup(GloveRenderer* render, string target_directory);

    // Returns a FullHandPoseObject min_pose incremented by incremental_pose.
    // Populates manifest object
    // Needs trained GloveTrack object to create classifiication images
    // Due to limitations in 3rd party library libhand, we are unfortunately forced to overwrite max_pose
    //TODO(shasheene@gmail.com): Add copy constructor to libhand headers
    void interpolate(int num_camera_angles, int num_poses_per_camera_angle, FullHandPose min_pose, FullHandPose incremental_pose, Manifest &manifest, GloveTrack glove_track);

private:
    // Returns a FullHandPose object with the difference between two poses, divided by num_poses.
    // WARNING: Due to limitations in 3rd party library libhand, currently overwrite max_pose. It's
    // not convenient to construct a new object without loading a YML file from disk.
    //TODO(shasheene@gmail.com): Modify libhand headers to allow copy constructor for FullHandPose
    FullHandPose CalculateIncrementalDifference(float num_poses, FullHandPose min_pose, FullHandPose max_pose);

    GloveRenderer* glove_renderer;
    string target_directory;
    FullHandPose IncrementPose(FullHandPose start, FullHandPose increment);

    string GenerateFilename(string target_directory, int index, int padding_zeroes);
    string GenerateTimestamp();
};

#endif

