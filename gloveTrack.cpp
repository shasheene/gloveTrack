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
int thresholdBrightness;

Mat frame;

Scalar classificationColor[NUMGLOVECOLORS];
int classificationArrayIndex;//used in mouse call back

//Debug and helper function
bool openCaptureDevice(VideoCapture &captureDevice, int deviceNumber);
void drawCurrentClassificationColors(Mat &targetFrame);//draws vertical squares representing classifcation color

bool realTimeMode = true;
int videoCaptureDeviceNumber = 0;

bool slowMode; //extra info above debug mode

int main(int argc, char** argv){
  debugMode=false;
  slowMode =false;
  parseCommandLineArgs(argc,argv);
  std::string trainingImagePath("db/blenderImg/");
  std::string testingImagePath("db/test/");

  namedWindow("gloveTrack", 1);
  setMouseCallback("gloveTrack", mouseCallback, NULL);

  if (realTimeMode==false){
    /*classificationColor[0] = Scalar(37, 20, 114, 0);//red
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

    //Size of reduced dimensionality image
    int databaseImageWidth = 50;
    int databaseImageHeight = 50;

    //Load image database
    //loadImageDatabase(comparisonImages, trainingImagePath, 64); //correct way around
    //loadImageDatabase(testingImages, testingImagePath, 64);
    loadImageDatabase(comparisonImages, testingImagePath, 64); //wrong for testing
    loadImageDatabase(testingImages, trainingImagePath, 64);
  
    for (int i=0;i<testingImages.size();i++){
      std::cerr << "Testing image number " << i << std::endl;

      //Normalize query:
      imshow("gloveTrack",testingImages.at(i));  
      waitKey(0);

      //Output X nearest neighbors by weighted hamming distance, 
      /*std::vector<int> nearestNeighboors = queryDatabasePose(normalizedQueryImage);

      for (int i=0;i<nearestNeighboors.size();i++) {
	std::cout << nearestNeighboors.at(i) << " ";
	}*/

      std::cout << "\nWaiting for user input before moving  to next image " << std::endl;
      waitKey(0);
    }
  } else {
    classificationColor[0] = Scalar(82, 35, 42, 0);
    classificationColor[1] = Scalar(20, 8, 155, 0);
    classificationColor[2] = Scalar(20, 20, 38, 0);
    classificationColor[3] = Scalar(89, 95, 10, 0);
    classificationColor[4] = Scalar(41, 130, 197, 0);
    classificationColor[5] = Scalar(144, 143, 85, 0);
    classificationColor[6] = Scalar(123, 79, 151, 0);
    classificationColor[6] = Scalar(255, 255, 255, 0); //black temporarily (should be glove color/negative space)
    int thresholdBrightness=64;

    std::string databaseImagePath("db/blenderImg");//query image path - temp

    VideoCapture captureDevice;
    openCaptureDevice(captureDevice, videoCaptureDeviceNumber); //videoCaptureDeviceNumber from -c argument
    if (!captureDevice.isOpened()) {
      std::cerr << " Unable to open video capture device " << videoCaptureDeviceNumber << " too. Quitting" << std::endl;
      exit(1);
    }
  
    iWidth = captureDevice.get(CV_CAP_PROP_FRAME_WIDTH);
    iHeight = captureDevice.get(CV_CAP_PROP_FRAME_HEIGHT);
  
    //Size of reduced dimensionality image
    int databaseImageWidth = 50;
    int databaseImageHeight = 50;

    //Load image database
    int initialImageDatabaseSize = loadImageDatabase(comparisonImages, databaseImagePath, 64);
  
    //Later load classificationColors from image file

    //Untouched background for calibration. Currently REQUIRING white background and doing no computation with it
    Mat backgroundFrame = captureFrame(captureDevice);

    while( true ) {
      double t = (double)getTickCount(); //fps calculation
      frame = captureFrame(captureDevice);

      //In future, do image processing here
      //Mat backgroundRemovalFrame = backgroundFrame(gloveBoundingBox);
      //currentFrame = cleanupImage(currentFrame, backgroundRemovalFrame); //Returns image classified into colors. All the smarts (and slowness) here

      Mat currentFrame = frame;
      Mat shrunkFrame = normalizeQueryImage(frame, thresholdBrightness);

      
     if (slowMode == true){
       //draw on screen (later debug only)
       Rect currentFrameScreenLocation(Point(40,40), currentFrame.size());
       currentFrame.copyTo(frame(currentFrameScreenLocation));
     }

      //Second run over image for faster lookup later. (May merge with cleanup)
      //Mat shrunkFrame = reduceDimensions(currentFrame, 50, 50);
     Rect shrunkFrameScreenLocation(Point(0,0), shrunkFrame.size()); //Draw shrunkFrame on given point on screen (later only in debug mode)
      shrunkFrame.copyTo(frame(shrunkFrameScreenLocation));
    
      if (comparisonImages.size() > 0){
	std::vector<int> indexOfMatch = queryDatabasePose(currentFrame);
	Rect roi(Point(100,240), comparisonImages.at(indexOfMatch.at(0)).size());
	//Isolate below into "getPoseImage()" later:
	comparisonImages.at(indexOfMatch.at(0)).copyTo(frame(roi));
      }
    
      if (debugMode==true){
	drawCurrentClassificationColors(frame); 
      }
  
      //READ KEYBOARD
      int c = waitKey(1);
      switch(c) {
      case 'p':
	std::cerr << "P pressed. Pushing back photo number " <<  numImagesTaken << " into " << numImagesTaken+initialImageDatabaseSize << std::endl;
	comparisonImages.push_back(currentFrame);//immediately make new comparison image this photo
	numImagesTaken++;
	break;
      case '[':
	if (thresholdBrightness>0){
	  thresholdBrightness--;
	}
	std::cerr << "Threshold brightness now: " << thresholdBrightness << std::endl;
	break;
      case ']':
	if (thresholdBrightness<255){
	  thresholdBrightness++;
	}
	std::cerr << "Threshold brightness now: " << thresholdBrightness << std::endl;
	break;
      case 'q':
	if (debugMode==true) {
	  //Backup unsaved comparison image files:
	  saveDatabase(comparisonImages, initialImageDatabaseSize,  databaseImagePath);
	  
	  //Later, save classification colors to image file
	  for (int i=0; i< NUMGLOVECOLORS; i++){
	    std::cerr << classificationColor[i] <<std::endl;
	  }
	}
	exit(0);
	break;
      }
      
      imshow("gloveTrack",frame);
      t = ((double)getTickCount() - t)/getTickFrequency();
      if(debugMode==true){std::cout << "Times passed in seconds: " << t << std::endl;}
    }
  }
  return (0);
}

//for debug:
void drawCurrentClassificationColors(Mat &frame) {
  Rect classificationRect = Rect( iWidth - 25 , (iHeight/20.0), 25, 45); //area to output classification colors
  
  //draw little square to show which color is being calibrated
  Rect selectorSymbolRect = Rect(iWidth - classificationRect.width/2 - 25, (iHeight/20.0)+ classificationArrayIndex*classificationRect.height +classificationRect.height/2, 10, 10); //nicely located to the left of the classification colors
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

Mat captureFrame(VideoCapture device) {
  Mat frame;
  bool readable = device.read(frame);
  if( !readable) {
    std::cerr << "Cannot read frame from video stream" << std::endl;
    exit(1);
  }
  return (frame);
}

  


void parseCommandLineArgs(int argc, char** argv) { //getopt
  char *cvalue = NULL;
  int index;
  int c;

  opterr = 0;
  while ((c = getopt (argc, argv, "vhdnsc:")) != -1)
    switch (c)
      {
      case 'v':
	std::cout << "gloveTrack" << " Version " << Glovetrack_VERSION_MAJOR
		  << " " << Glovetrack_VERSION_MINOR << std::endl;
	exit(0);
        break;
      case 'h':
	std::cout << "Usage: ./gloveTrack [-d|-v]" << std::endl;
	exit(0);
        break;
      case 'd':
	std::cout << "Debug mode enabled" << std::endl;
	debugMode=true;
        break;
      case 'n':
	std::cout << "Nearest neighbor test mode(Realtime disabled)" << std::endl;
	realTimeMode = false;
        break;
      case 's':
	std::cout << "Slow mode enabled (even more info than debug mdoe" << std::endl;
	slowMode = true;
        break;
      case 'c':
	videoCaptureDeviceNumber = atoi(optarg);
	fprintf (stderr, "Video capture device selected is %d\n", videoCaptureDeviceNumber);
        break;
      case '?':
        if (optopt == 'c')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        exit(1);
      default:
        abort ();
      }
  printf ("Video capture device number is %s\n",cvalue);

  for (index = optind; index < argc; index++)
    printf ("Non-option argument %s\n", argv[index]);

}
