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
		  (int)(saturate_cast<uchar>(alpha*frame.ptr<uchar>(y)[x*3+0]+beta)), 
		  (int)(saturate_cast<uchar>(alpha*frame.ptr<uchar>(y)[x*3+1]+beta)), 
		  (int)(saturate_cast<uchar>(alpha*frame.ptr<uchar>(y)[x*3+2]+beta)));
 	 std::cerr << "Calibrated " << calibrationIndex << " with " << calibrationColor[calibrationIndex] << std::endl;
	 calibrationIndex = (calibrationIndex+1)%NUMGLOVECOLORS;
       }
     }
     else if  ( event == EVENT_RBUTTONDOWN )
     {
       calibrationIndex--;
       if (calibrationIndex <0) {
	 calibrationIndex = NUMGLOVECOLORS -1;
       }
       std::cerr << "calibrationIndex now " << calibrationIndex << std::endl;
     }
     else if  ( event == EVENT_MBUTTONDOWN )
     {
          std::cerr << "Middle button of the mouse is clicked - position (" << x << ", " << y << ")" << std::endl;
     }
     else if ( event == EVENT_MOUSEMOVE )
     {
       //std::cerr << "Mouse move over the window - position (" << x << ", " << y << ")" << std::endl;

     }
}
