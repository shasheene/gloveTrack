#ifndef LOOKUPDATABASE_H
#define LOOKUPDATABASE_H

#include "libsAndConst.h"

int queryDatabasePose(Mat isolatedFrame);

void increaseBrightnessAndConstrastOfPixel(Mat frame, int row, int col);


//Takes webcam image and background image and returns image ready for lookup 
Mat cleanupImage(Mat isolatedFrame, Mat isolatedBackgroundFrame);//same size images


#endif
