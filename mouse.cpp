#include "mouse.h"

//see libsAndConst for extern'd global variables.

void mouseCallback(int event, int x, int y, int flags, void* userdata)
{
     if  ( event == EVENT_LBUTTONDOWN )
     {

       //std::cerr << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << std::endl;
       if (debugMode==true) {
	 
	 classificationColor[classificationArrayIndex] = 
	   Scalar(
		  (int)(saturate_cast<uchar>(ALPHA*frame.ptr<uchar>(y)[x*3+0]+BETA)), //ALPHA/BETA #define in libAndConst file
		  (int)(saturate_cast<uchar>(ALPHA*frame.ptr<uchar>(y)[x*3+1]+BETA)), 
		  (int)(saturate_cast<uchar>(ALPHA*frame.ptr<uchar>(y)[x*3+2]+BETA)));
 	 std::cerr << "Calibrated " << classificationArrayIndex << " with " << classificationColor[classificationArrayIndex] << std::endl;
       }
     }
     else if  ( event == EVENT_RBUTTONDOWN )
     {
       classificationArrayIndex = (classificationArrayIndex+1)%NUMGLOVECOLORS;
       std::cerr << "classificationArrayIndex now " << classificationArrayIndex << std::endl;
     }
     else if  ( event == EVENT_MBUTTONDOWN )
     {
       classificationArrayIndex--;
       if (classificationArrayIndex <0) {
	 classificationArrayIndex = NUMGLOVECOLORS -1;
       }
       std::cerr << "classificationArrayIndex now " << classificationArrayIndex << std::endl;
     }
     else if ( event == EVENT_MOUSEMOVE )
     {
       //std::cerr << "Mouse move over the window - position (" << x << ", " << y << ")" << std::endl;

     }
}
