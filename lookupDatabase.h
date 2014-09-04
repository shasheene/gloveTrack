#ifndef LOOKUPDATABASE_H
#define LOOKUPDATABASE_H

#include "libsAndConst.h"

/*
Potential future public API:
int* getPostRawData();// returns int array of finger/hand/etc position for another program
Mat getPoseRawImage(Mat isolatedFrame);
int getPose3DModel();//return index to 3D model DB
*/

//If public API exist, these will be private functions:
int queryDatabasePose(Mat isolatedFrame);
void increaseBrightnessAndConstrastOfPixel(Mat frame, int row, int col);

//Takes webcam image and background image and returns image ready for lookup 
Mat cleanupImage(Mat isolatedFrame, Mat isolatedBackgroundFrame);//same size images

#endif
