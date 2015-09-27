#include "mouse.hpp"

//see libsAndConst for extern'd global variables.

void mouseCallback(int event, int x, int y, int flags, void* userdata) {
    auto console = spdlog::get("console");

    if (event == EVENT_LBUTTONDOWN) {

        SPDLOG_TRACE(console, "Left button of the mouse is clicked - position {}, {}", x, y);
        /*
          classificationColor[classificationArrayIndex] = 
            Scalar(
                   (int)(saturate_cast<uchar>(ALPHA*frame.ptr<uchar>(y)[x*3+0]+BETA)), //ALPHA/BETA #define in libAndConst file
                   (int)(saturate_cast<uchar>(ALPHA*frame.ptr<uchar>(y)[x*3+1]+BETA)), 
                   (int)(saturate_cast<uchar>(ALPHA*frame.ptr<uchar>(y)[x*3+2]+BETA)));
          std::cerr << "Calibrated " << classificationArrayIndex << " with " << classificationColor[classificationArrayIndex] << std::endl;
        }
         */
    } else if (event == EVENT_RBUTTONDOWN) {
        classificationArrayIndex = (classificationArrayIndex + 1) % NUMGLOVECOLORS;
        console->info("classificationArrayIndex now {}", classificationArrayIndex);
    } else if (event == EVENT_MBUTTONDOWN) {
        classificationArrayIndex--;
        if (classificationArrayIndex < 0) {
            classificationArrayIndex = NUMGLOVECOLORS - 1;
        }
        console->info("classificationArrayIndex now {}", classificationArrayIndex);
    } else if (event == EVENT_MOUSEMOVE) {
        //std::cerr << "Mouse move over the window - position (" << x << ", " << y << ")" << std::endl;

    }
}
