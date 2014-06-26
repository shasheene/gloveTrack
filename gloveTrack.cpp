#include "gloveTrack.h"

using namespace cv;

int detection(Mat);
Rect boundingBox;
int numImagesTaken = 0;

std::vector<Mat> comparisonImages;

int main(int argc, char** argv){
  if (argc > 1){
    /*if (argv[1].("-v")==0) {
      std::cout << argv[0] << " Version " << Glovetrack_VERSION_MAJOR
	 << " " << Glovetrack_VERSION_MINOR << std::endl;
    }
    if (argv[1].("-h")==0) {
    std::cout << "usage: DisplayImage.out <Image_Path>" << std::endl;
    return 1;
    }*/
  }
  Mat frame;
  
  VideoCapture capture(0);
  if (!capture.isOpened()) {
    std::cout << " Unable to open video capture device";
    return (1);
  }

  double iWidth = capture.get(CV_CAP_PROP_FRAME_WIDTH);
  double iHeight = capture.get(CV_CAP_PROP_FRAME_HEIGHT);

  boundingBox = Rect( (iWidth/2.0), (iHeight/2.0), 100, 100);

  
  for (int i=0;i<NUMTRAININGIMAGES;i++) {
    std::string start("db/trainingSet");
    std::string end (".jpg");
    std::stringstream ss;
    ss << start << i << end;
    std::string imgInputPath(ss.str());

    std::cerr << imgInputPath;
    Mat trainImg = imread(imgInputPath,1); //later make it non-hardcoded. 
    comparisonImages.push_back(trainImg.clone());//img already correct size, so no need to boundbox
  }

  while( true ) {
    double t = (double)getTickCount();
    bool readable = capture.read(frame);
    if( !readable) {
      std::cout << "Cannot read frame from video stream";
      return 1;
    }

    rectangle(frame, boundingBox, Scalar(0,0,0)); //draw rect
    Mat currentFrame = frame(boundingBox);
    int indexOfMatch = detection(currentFrame);

    Rect roi(Point(100,240), comparisonImages.at(indexOfMatch).size());
    comparisonImages.at(indexOfMatch).copyTo(frame(roi));
    
    int c = waitKey(10);
    if( (char)c == 'c' ) { break; }

    if( (char)c == 'p' ) {
      Mat photo;
      capture.read(photo);//ERROR CHECKING!
      photo = photo(boundingBox);
      std::cout << "P pressed. Taking photo "<< std::endl;
      std::string start("db/trainingSet");
      std::string end (".jpg");
      std::stringstream ss;
      ss << start << numImagesTaken  << end;
      std::string imgOutputPath(ss.str());
      imwrite( imgOutputPath, photo );
      comparisonImages.push_back(photo);//immediately make new comparison image this photo
      numImagesTaken++;
    }

    imshow("gloveTrack",frame);
    t = ((double)getTickCount() - t)/getTickFrequency();
    std::cout << "Times passed in seconds: " << t << std::endl;
  }
  return (0);
}

int detection(Mat curr) {
  int rows = curr.rows;
  int cols = curr.cols;

  int indexOfSmallestHamming = -1;
  int smallestHamming = curr.rows * curr.cols + 1;
  //std::cerr << smallestHamming << std::endl;

  for (int k=0;k<comparisonImages.size();k++) {
    int runningTotalHammingDist = 0;
    for (int i=0;i<rows;i++){
      for (int j=0;j<cols;j++){
	if ( comparisonImages.at(k).at<uchar>(i,j) != curr.at<uchar>(i,j) ){ //recall uchar is 8 bit, 0->255
	  runningTotalHammingDist++;
	}
      }
    }

    //Determine lowest
    if (runningTotalHammingDist < smallestHamming) {
      smallestHamming = runningTotalHammingDist;
      indexOfSmallestHamming = k;
    }
  }
  return indexOfSmallestHamming;
}
