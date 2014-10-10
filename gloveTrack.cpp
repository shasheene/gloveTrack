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
  /*
  classificationColor[0] = Scalar(37, 20, 114, 0);//red
  classificationColor[1] = Scalar(29, 15, 94, 0);//green
  classificationColor[2] = Scalar(68, 20, 38, 0);//dark blue
  classificationColor[3] = Scalar(28, 28, 50, 0);//black (brown on blender)
  classificationColor[4] = Scalar(51, 40, 106, 0);//orange (yellow on blender)
  classificationColor[5] = Scalar(51, 63, 120, 0);//light blue
  classificationColor[6] = Scalar(45, 79, 86, 0);//purple
  classificationColor[7] = Scalar(255, 253, 255, 0);//pink (white in blender)
  classificationColor[8] = Scalar(71, 67, 109, 0);//  
  */

  //Actual blender cols:
  classificationColor[0] = Scalar(0, 0, 0, 255);//background (black on blender)
  classificationColor[1] = Scalar(42, 24, 168, 0);//red
  classificationColor[2] = Scalar(57, 81, 35, 0);//green
  classificationColor[3] = Scalar(68, 40, 46, 0);//dark blue
  classificationColor[4] = Scalar(28, 28, 50, 0);//black (brown on blender)
  classificationColor[5] = Scalar(42, 223, 255, 0);//orange (yellow on blender)
  classificationColor[6] = Scalar(255, 255, 90, 0);//lightblue
  classificationColor[7] = Scalar(101, 55, 155, 0);//bright purple
  classificationColor[8] = Scalar(255, 251, 255, 0); //pink (white in blender)

  /*Old webcam:
  blenderGloveColor[0] = Scalar(37, 20, 114, 0);//red
  blenderGloveColor[1] = Scalar(29, 15, 94, 0);//green
  blenderGloveColor[2] = Scalar(20, 20, 38, 0);//blue
  blenderGloveColor[3] = Scalar(41, 60, 24, 0);//black
  blenderGloveColor[4] = Scalar(51, 40, 106, 0);//orange
  blenderGloveColor[5] = Scalar(51, 63, 120, 0);//light blue
  blenderGloveColor[6] = Scalar(45, 79, 86, 0);//purple
  blenderGloveColor[7] = Scalar(76, 40, 118, 0);//pink
  blenderGloveColor[8] = Scalar(71, 67, 109, 0);//
  */
  debugMode=false;
  std::string trainingImagePath("db/blenderImg/");
  std::string testingImagePath("db/test/");
  parseCommandLineArgs(argc,argv);

  namedWindow("gloveTrack", 1);
  setMouseCallback("gloveTrack", mouseCallback, NULL);

  //Size of reduced dimensionality image
  int databaseImageWidth = 50;
  int databaseImageHeight = 50;

  //Load image database
  loadImageDatabase(comparisonImages, trainingImagePath);
  loadImageDatabase(testingImages, testingImagePath);
  
  for (int i=0;i<testingImages.size();i++){
    std::cerr << "Testing image number " << i << std::endl;

    //Normalize query:
    Mat normalizedQueryImage = normalizeQueryImage(testingImages.at(i));
    imshow("gloveTrack",normalizedQueryImage);  
    waitKey(0);

    //Output X nearest neighbors by weighted hamming distance, 
    std::vector<int> nearestNeighboors = queryDatabasePose(normalizedQueryImage);

    for (int i=0;i<nearestNeighboors.size();i++) {
      std::cout << nearestNeighboors.at(i) << " ";
    }

    std::cout << "\nWaiting for user input before moving  to next image " << std::endl;
    waitKey(0);
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
