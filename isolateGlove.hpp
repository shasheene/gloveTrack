#ifndef ISOLATEGLOVE_H
#define ISOLATEGLOVE_H

#include "libsAndConst.hpp"
#include "lookupDatabase.hpp"
#include "commandLineArguments.hpp"

class GloveTrack {
public:
    GloveTrack();
    void Setup(vector<Mat> unlabelled_training_images, vector<Mat> labelled_training_images, Scalar glove_colors[NUMGLOVECOLORS], LookupDb lookup_db, struct arguments arg_struct);

    Mat GetLastNormalizedImage();
    
    //TODO(shasheeene@gmail.com): Make this return handpose object
    vector<int> GetHandPose(Mat unnormalized_image);

private:
    int result_to_index[NUMGLOVECOLORS];
    Scalar glove_colors[NUMGLOVECOLORS];
    EM em;
    
    Mat last_normalized_image;
    LookupDb lookup_db;
    
    // Returns image with colors classified into buckets Uses bilateral filter, expectation maximization (Guassian Mixture Model) for color classifications, meanshift (crop).
    Mat Normalize(Mat unnormalized_image);
    //TODO(shasheeene@gmail.com): delete this line before commit Mat normalizeQueryImage(Mat& unprocessedCameraFrame, EM& trainedEM, int (&resultToIndex)[NUMGLOVECOLORS], struct arguments args);

    //TODO(shasheeene@gmail.com): fix API
    //Pass in array of training images (currently not array though)
    bool TrainExpectationMaximizationModel(vector<Mat> raw_training_images, vector<Mat> labelled_training_images);

    //TODO WHAT? Compares each pixel of prelabelledSampleArray to classificationColor array and trains a classifier  normal ("g (Guassian Mixture Model)
    void ConvertLabelledToEMInitialTrainingProbabilityMatrix(Mat pre_labelled_sample_array, Mat& prob, int num_clusters_in_em);

    //Rasterize/Convert 2D BGR image matrix (MxN size) to a 1 dimension "sample vector" matrix, where each is a BGR pixel (so, 1x(MxN) size). This is the required format for OpenCV algorithms
    Mat ConvertToSampleArray(Mat frame, Mat& output_sample_array);

    
    void MeanShiftCrop(Mat frame, int maximum_iterations, int minimum_distance, struct arguments args);

    //Perform segmentation (clustering) using EM algorithm
    void ClassifyColors(Mat test_image, Mat test_image_sample_array, Mat& output_array);
    // TODO(shasheeene@gmail.com): fix this line before commit void classifyColors(Mat test_image, Mat test_image_sample_array, Mat& output_array, EM& em, int (&result_to_index)[NUMGLOVECOLORS]);
    
    //TODO(shasheeene@gmail.com): remove before commiting
    struct arguments args;
};

//Takes camera frame (from decent camera - not just bad color webcam) and returns image ready for query
Mat fastNormalizeQueryImage(Mat unprocessed_camera_frame, int dark_threshold);
Mat tempNormalizeCamera(Mat unprocessed_camera_frame, int threshold_brightness);
Mat fastClassifyColors(Mat cropped_image);
Mat classifyCamera(Mat cropped_image); //temp
//Below should be obselete because only want one cycle over image during online processing

Mat meanShiftCrop(Mat frame, int maximum_iterations, int minimum_distance, struct arguments args);

//Returns position of glove as cv::Rect
Rect fastLocateGlove(Mat region, int dark_threshold);

//Scales image down for faster lookup. Currenly just throw away pixels
Mat fastReduceDimensions(Mat region, int percent_scaling);
#endif // ISOLATEGLOVE_H
