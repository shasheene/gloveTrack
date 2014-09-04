
#ifndef INCLUDED_LIBS_AND_CONST
#define  INCLUDED_LIBS_AND_CONST

#include <opencv2/opencv.hpp>
#include <opencv/highgui.h> //otherwise ‘CV_WINDOW_AUTOSIZE’ undeclared error

using namespace cv;


//globals set in gloveTrack.cpp
extern double iWidth;
extern double iHeight;
extern bool debugMode;
extern std::vector<Mat> comparisonImages;

extern Mat frame;

extern Scalar calibrationColor[];
#define NUMGLOVECOLORS 9
extern int calibrationIndex;

//brightness changes
#define ALPHA 1 //1.3
#define BETA 0 //15

extern void increaseBrightnessAndConstrastOfPixel(Mat frame, int row, int col);

extern Mat cleanupImage(Mat isolatedFrame, Mat isolatedBackgroundFrame);//same size image

#endif
