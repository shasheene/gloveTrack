#ifndef ISOLATEGLOVE_H
#define ISOLATEGLOVE_H

#include "libsAndConst.hpp"

//Full normalization methods - (bilateral filter, expectation maximization (Guassian Mixture Model) for color classifications, meanshift (crop).
Mat normalizeQueryImage(Mat& unprocessedCameraFrame, EM& trainedEM,
        int (&resultToIndex)[NUMGLOVECOLORS], struct arguments args);

/**
Perform segmentation (clustering) using EM algorithm
 **/
void classifyColors(Mat testImage, Mat testImageSampleArray, Mat& outputArray, EM& em, int (&resultToIndex)[NUMGLOVECOLORS]);

//Pass in array of training images (currently not array though)
bool trainExpectationMaximizationModel(Mat rawTrainingImages[], Mat labelledTrainingImages[], int numTrainingImages, EM& em, int (&resultToIndex)[NUMGLOVECOLORS]);

//Compares each pixel of prelabelledSampleArray to classificationColor array and trains a classifier  normal ("g (Guassian Mixture Model)
void convertLabelledToEMInitialTrainingProbabilityMatrix(Mat prelabelledSampleArray, Mat& prob, int numClustersInEM);


//Rasterize/Convert 2D BGR image matrix (MxN size) to a 1 dimension "sample vector" matrix, where each is a BGR pixel (so, 1x(MxN) size). This is the required format for OpenCV algorithms
Mat convertToSampleArray(Mat frame, Mat& outputSampleArray);


//Takes camera frame (from decent camera - not just bad color webcam) and returns image ready for query
Mat fastNormalizeQueryImage(Mat unprocessedCameraFrame, int darkThreshold);
Mat tempNormalizeCamera(Mat unprocessedCameraFrame, int thresholdBrightness);

Mat fastClassifyColors(Mat croppedImage);
Mat classifyCamera(Mat croppedImage); //temp

//Below should be obselete because only want one cycle over image during online processing


Mat meanShiftCrop(Mat frame, int maximumIterations, int minimumDistance, struct arguments args);

//Returns position of glove as cv::Rect
Rect fastLocateGlove(Mat region, int darkThreshold);


//Scales image down for faster lookup. Currenly just throw away pixels
Mat fastReduceDimensions(Mat region, int percentScaling);
#endif
