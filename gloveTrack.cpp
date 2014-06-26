#include "gloveTrack.h"

using namespace cv;

void detection(Mat);

int main(int argc, char** argv){
  if (argc > 1){
    if (strcmp(argv[1],"-v")==0) {
      std::cout << argv[0] << " Version " << Glovetrack_VERSION_MAJOR
	 << " " << Glovetrack_VERSION_MINOR << std::endl;
    }
    if (strcmp(argv[1],"-h")==0) {
    std::cout << "usage: DisplayImage.out <Image_Path>" << std::endl;
    return 1;
    }
  }
  Mat frame;
  VideoCapture capture(0);
  if (!capture.isOpened()) {
    std::cout << " Unable to open video capture device";
    return (1);
  }

  double iWidth = capture.get(CV_CAP_PROP_FRAME_WIDTH);
  double iHeight = capture.get(CV_CAP_PROP_FRAME_HEIGHT);

  while( true ) {
    bool readable = capture.read(frame);
    if( !readable) {
      std::cout << "Cannot read frame from video stream";
      return 1;
    }

    detection(frame);

    
    int c = waitKey(10);
    if( (char)c == 'c' ) { break; }
  
  }
  return (0);
}

void detection( Mat frame ) {
  imshow("gloveTrack",frame);
}
