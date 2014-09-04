#include "mouse.h"

//see libsAndConst for extern'd global variables.

void mouseCallback(int event, int x, int y, int flags, void* userdata)
{
     if  ( event == EVENT_LBUTTONDOWN )
     {

       //std::cerr << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << std::endl;
       if (debugMode==true) {
	 
	 calibrationColor[calibrationIndex] = 
	   Scalar(
		  (int)(saturate_cast<uchar>(ALPHA*frame.ptr<uchar>(y)[x*3+0]+BETA)), //ALPHA/BETA #define in libAndConst file
		  (int)(saturate_cast<uchar>(ALPHA*frame.ptr<uchar>(y)[x*3+1]+BETA)), 
		  (int)(saturate_cast<uchar>(ALPHA*frame.ptr<uchar>(y)[x*3+2]+BETA)));
 	 std::cerr << "Calibrated " << calibrationIndex << " with " << calibrationColor[calibrationIndex] << std::endl;
       }
     }
     else if  ( event == EVENT_RBUTTONDOWN )
     {
       calibrationIndex = (calibrationIndex+1)%NUMGLOVECOLORS;
       std::cerr << "calibrationIndex now " << calibrationIndex << std::endl;
     }
     else if  ( event == EVENT_MBUTTONDOWN )
     {
       calibrationIndex--;
       if (calibrationIndex <0) {
	 calibrationIndex = NUMGLOVECOLORS -1;
       }
       std::cerr << "calibrationIndex now " << calibrationIndex << std::endl;
     }
     else if ( event == EVENT_MOUSEMOVE )
     {
       //std::cerr << "Mouse move over the window - position (" << x << ", " << y << ")" << std::endl;

     }
}
