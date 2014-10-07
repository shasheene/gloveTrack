#include "gloveTrack.h"

void parseCommandLineArgs(int, char**);
std::string concatStringInt(std::string part1,int part2);

Mat captureFrame(VideoCapture device);//takes photo and returns it
int numImagesTaken = 0;

//Globals (declared extern'd in libsAndConst.h and defined mostly in main)
bool debugMode;
std::vector<Mat> comparisonImages;
std::vector<Mat> testingImages;
double iWidth, iHeight;

Mat frame;

Scalar classificationColor[NUMGLOVECOLORS];
int classificationArrayIndex;//used in mouse call back

//Debug and helper function
bool openCaptureDevice(VideoCapture &captureDevice, int deviceNumber);
void drawCurrentClassificationColors(Mat &targetFrame);//draws vertical squares representing classifcation color

  
int main(int argc, char** argv){
  int videoCaptureDeviceNumber = 1;
  
  classificationColor[0] = Scalar(50, 28, 33, 0);
  classificationColor[1] = Scalar(29, 15, 94, 0);
  classificationColor[2] = Scalar(20, 20, 38, 0);
  classificationColor[3] = Scalar(41, 60, 24, 0);
  classificationColor[4] = Scalar(51, 40, 106, 0);
  classificationColor[5] = Scalar(51, 63, 120, 0);
  classificationColor[6] = Scalar(45, 79, 86, 0);
  classificationColor[7] = Scalar(76, 40, 118, 0);
  classificationColor[8] = Scalar(71, 67, 109, 0);

  debugMode=false;
  std::string trainingImagePath("db/trainingSet");
  std::string testingImagePath("db/testingSet");
  parseCommandLineArgs(argc,argv);

  namedWindow("gloveTrack", 1);
  setMouseCallback("gloveTrack", mouseCallback, NULL);

  //Size of reduced dimensionality image
  int databaseImageWidth = 50;
  int databaseImageHeight = 50;

  //Load image database
  int initialImageDatabaseSize = loadImageDatabase(comparisonImages, trainingImagePath);
  int testingImageDatabaseSize = loadImageDatabase(testingImages, testingImagePath);
  
  int test;
  for (int i=0;i<testingImageDatabaseSize;i++){
    std::cout << "Testing image number " << i << std::endl;

    //Output X nearest neighbors by weighted hamming distance, 
    vector<int> nearestNeighboors = queryDatabasePose(testingImages.at(i));

    for (int i=0;i<nearestNeighboors.size();i++) {
      std::cout << nearestNeighboors.at(i) << " ";
    }
    std::cout << "\nWaiting for user input before moving  to next image " << std::endl;

    std::cin >> test; //pause for Enter
  }
  

  return (0);
}

//for debug:
void drawCurrentClassificationColors(Mat &frame) {
  Rect classificationRect = Rect( (iWidth/2.0), (iHeight/20.0), 25, 45); //area to output classification colors
  
  //draw little square to show which color is being calibrated
  Rect selectorSymbolRect = Rect( (iWidth/2.0) - classificationRect.width/2, (iHeight/20.0)+ classificationArrayIndex*classificationRect.height +classificationRect.height/2, 10, 10); //nicely located to the left of the classification colors
  Mat selectorSymbol(frame,selectorSymbolRect);
  selectorSymbol = Scalar(0,0,0,0);//black square;
  selectorSymbol.copyTo(frame(selectorSymbolRect));
  
  //draw actual classification colors on screen
  for (int i=0;i<NUMGLOVECOLORS;i++) {
    Mat smallBlockOfColor(frame, classificationRect);
    smallBlockOfColor = classificationColor[i];
    smallBlockOfColor.copyTo(frame(classificationRect));
    classificationRect.y += classificationRect.height;
  }
}
  
bool openCaptureDevice(VideoCapture &captureDevice, int deviceNumber) {
    captureDevice.open(deviceNumber);
    return (captureDevice.isOpened());
}

  


void parseCommandLineArgs(int argc, char** argv) {
  if (argc > 1){
    if (strcmp(argv[1],"-v")==0) {
      std::cout << "gloveTrack" << " Version " << Glovetrack_VERSION_MAJOR
		<< " " << Glovetrack_VERSION_MINOR << std::endl;
      exit(0);
    }

    if (strcmp(argv[1],"-h")==0) {
      std::cout << "Usage: ./gloveTrack [-d|-v]" << std::endl;
      exit(0);
    }

    if (strcmp(argv[1],"-d")==0) {
      std::cout << "Debug mode enabled" << std::endl;
      debugMode=true;
    }
  }
}
