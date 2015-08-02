#ifndef INCLUDED_LIBS_AND_CONST
#define  INCLUDED_LIBS_AND_CONST

#include <opencv2/opencv.hpp>
#include <opencv/highgui.h> //otherwise ‘CV_WINDOW_AUTOSIZE’ undeclared error

using namespace cv;


//globals set in gloveTrack.cpp. Will remove at some point
extern double image_width;
extern double image_height;
extern int verbosity;
extern std::vector<Mat> comparisonImages;

extern int thresholdBrightness;

extern Mat frame; // Entire camera frame


#define NUMGLOVECOLORS 9 //8 unique colors, and index 0 being temp backgound removal
extern Scalar classificationColor[];//camera image classification buckets
extern Scalar blenderGloveColor[];//color buckets of rendered blender glove images (same indexing as above)
extern int classificationArrayIndex; //Used for calibration mode mouse.cpp events

//brightness and constrast changes
#define ALPHA 1 //1.3
#define BETA 0 //15

//We define some constants for internal processing
#define OUTPUT_HEIGHT 200
#define OUTPUT_WIDTH 200
#define INTERMEDIATE_HEIGHT 100
#define INTERMEDIATE_WIDTH 100
#define NORMALIZED_HEIGHT 40
#define NORMALIZED_WIDTH 40

extern void increaseBrightnessAndConstrastOfPixel(Mat frame, int row, int col);

extern Mat cleanupImage(Mat isolatedFrame, Mat isolatedBackgroundFrame);//same size image

#endif
