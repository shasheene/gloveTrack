
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

extern Scalar calibrationColor[];
extern const int numGloveColors;

#endif
