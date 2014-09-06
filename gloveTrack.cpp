#include "gloveTrack.h"

void parseCommandLineArgs(int, char**);
std::string concatStringInt(std::string part1,int part2);

Mat captureFrame(VideoCapture device);//takes photo and returns it
int numImagesTaken = 0;

//Globals (declared extern'd in libsAndConst.h and defined mostly in main)
bool debugMode;
std::vector<Mat> comparisonImages;
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
  std::string shrunkImagePath("db/trainingSet");
  std::string bigImagePath("db/big/bigSet");
  parseCommandLineArgs(argc,argv);

  namedWindow("gloveTrack", 1);
  setMouseCallback("gloveTrack", mouseCallback, NULL);

  VideoCapture captureDevice;

  //Open first successful device
  bool successfullyOpenedDevice = false;
  while (successfullyOpenedDevice==false && videoCaptureDeviceNumber<5) {
    if (!openCaptureDevice(captureDevice, videoCaptureDeviceNumber)) {
      std::cerr << " Unable to open video capture device #" << videoCaptureDeviceNumber << std::endl;
      captureDevice.release();
      videoCaptureDeviceNumber++;
    } else {
      successfullyOpenedDevice = true;
      std::cerr << " Opened device #" << videoCaptureDeviceNumber << " press V to cycle through all devices" << std::endl;
    }
  }
  
  //Trivial to add arg parsing for selection later
  if (!captureDevice.isOpened()) {
    std::cerr << " Unable to open video capture device 0 too. Quitting" << std::endl;
    exit(1);
  }
  
  
  
  iWidth = captureDevice.get(CV_CAP_PROP_FRAME_WIDTH);
  iHeight = captureDevice.get(CV_CAP_PROP_FRAME_HEIGHT);
  
  //Size of reduced dimensionality image
  int databaseImageWidth = 50;
  int databaseImageHeight = 50;

  //Load image database
  int initialImageDatabaseSize = loadImageDatabase(comparisonImages, shrunkImagePath);
  
  //Later load classificationColors from image file

  //Untouched background for calibration. Currently REQUIRING white background and doing no computation with it
  Mat backgroundFrame = captureFrame(captureDevice);

  while( true ) {
    double t = (double)getTickCount(); //fps calculation
    frame = captureFrame(captureDevice);
    
    Rect gloveRegion = locateGlove(frame); //No actual tracking yet (returns fixed region)
    rectangle(frame, gloveRegion, Scalar(0,0,0)); //Draw rectangle represententing tracked location
    
    Mat currentFrame = frame(gloveRegion);
    Mat backgroundRemovalFrame = backgroundFrame(gloveRegion);
    currentFrame = cleanupImage(currentFrame, backgroundRemovalFrame); //Returns image classified into colors. All the smarts (and slowness) here
    
    //draw on screen (later debug only)
    Rect currentFrameScreenLocation(Point(40,40), currentFrame.size());
    currentFrame.copyTo(frame(currentFrameScreenLocation));
    
    //Second run over image for faster lookup later. (May merge with cleanup)
    Mat shrunkFrame = reduceDimensions(currentFrame, 50, 50);
    Rect shrunkFrameScreenLocation(Point(0,0), shrunkFrame.size()); //Draw shrunkFrame on given point on screen (later only in debug mode)
    shrunkFrame.copyTo(frame(shrunkFrameScreenLocation));
    
    if (comparisonImages.size() > 0){
      int indexOfMatch = queryDatabasePose(currentFrame);
      Rect roi(Point(100,240), comparisonImages.at(indexOfMatch).size());
      //Isolate below into "getPoseImage()" later:
      comparisonImages.at(indexOfMatch).copyTo(frame(roi));
    }
    
    
    //READ KEYBOARD
    int c = waitKey(10);
    if( (char)c == 'p' ) {
      std::cerr << "P pressed. Pushing back photo number " <<  numImagesTaken << " into " << numImagesTaken+initialImageDatabaseSize << std::endl;
      comparisonImages.push_back(currentFrame);//immediately make new comparison image this photo
      numImagesTaken++;
    }

    if (debugMode==true){
      drawCurrentClassificationColors(frame); 
    }

    if( (char)c == 'q' ) {
      if (debugMode==true) {
	//Backup unsaved comparison image files:
	saveDatabase(comparisonImages, initialImageDatabaseSize,  shrunkImagePath);

	//Later, save classification colors to image file
      }
      exit(0);
    }
    
    imshow("gloveTrack",frame);
    t = ((double)getTickCount() - t)/getTickFrequency();
    if(debugMode==true){std::cout << "Times passed in seconds: " << t << std::endl;}
  }

  return (0);
}


Mat captureFrame(VideoCapture device) {
  Mat frame;
  bool readable = device.read(frame);
  if( !readable) {
    std::cerr << "Cannot read frame from video stream" << std::endl;
    exit(1);
  }
  return (frame);
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
