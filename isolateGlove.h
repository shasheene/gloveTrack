#ifndef ISOLATEGLOVE_H
#define ISOLATEGLOVE_H

#include "libsAndConst.h"

//Changes color values which locateGlove uses
void calibrate(Mat cameraFrame, Rect calibrationRect);

//Returns position of glove as cv::Rect
Rect locateGlove(Mat cameraFrame);

//Scales image down for faster lookup. Currenly just throw away pixels
Mat reduceDimensions(Mat region, int x, int y);


#endif
