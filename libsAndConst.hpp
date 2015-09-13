#ifndef INCLUDED_LIBS_AND_CONST
#define  INCLUDED_LIBS_AND_CONST

#include <opencv2/opencv.hpp>
#include <opencv/highgui.h> //otherwise ‘CV_WINDOW_AUTOSIZE’ undeclared error
#include "gloveTrackConfig.hpp"

// Header only library used for logging
#include "include/spdlog/spdlog.h"


using namespace cv;


//globals set in gloveTrack.cpp. Will remove at some point
extern int verbosity;
extern std::vector<Mat> comparisonImages;

extern bool realTimeMode;
extern int videoCaptureDeviceNumber;
extern bool slowMode;

extern int thresholdBrightness;

extern Mat frame; // Entire camera frame


#define NUMGLOVECOLORS 9 //8 unique colors, and index 0 being temp backgound removal
extern Scalar classificationColor[];//camera image classification buckets
extern Scalar blenderGloveColor[];//color buckets of rendered blender glove images (same indexing as above)
extern int classificationArrayIndex; //Used for calibration mode mouse.cpp events

//brightness and constrast changes
#define ALPHA 1 //1.3
#define BETA 0 //15

extern void increaseBrightnessAndConstrastOfPixel(Mat frame, int row, int col);

extern Mat cleanupImage(Mat isolatedFrame, Mat isolatedBackgroundFrame);//same size image

#endif
