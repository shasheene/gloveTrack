#ifndef ISOLATEGLOVE_H
#define ISOLATEGLOVE_H

#include "libsAndConst.h"

//Takes camera frame (from decent camera - not just bad color webcam) and returns image ready for query
Mat normalizeQueryImage(Mat unprocessedCameraFrame, int darkThreshold);
Mat tempNormalizeCamera(Mat unprocessedCameraFrame, int thresholdBrightness);

Mat classifyColors(Mat croppedImage);
Mat classifyCamera(Mat croppedImage);//temp

//Below should be obselete because only want one cycle over image during online processing

//Returns position of glove as cv::Rect
Rect locateGlove(Mat region, int darkThreshold);


//Scales image down for faster lookup. Currenly just throw away pixels
Mat reduceDimensions(Mat region, int x, int y);


#endif
