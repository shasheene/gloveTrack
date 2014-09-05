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

Scalar calibrationColor[NUMGLOVECOLORS];
int calibrationIndex;//used in mouse call back

int main(int argc, char** argv){
  calibrationColor[0] = Scalar(50, 28, 33, 0);
  calibrationColor[1] = Scalar(29, 15, 94, 0);
  calibrationColor[2] = Scalar(20, 20, 38, 0);
  calibrationColor[3] = Scalar(41, 60, 24, 0);
  calibrationColor[4] = Scalar(51, 40, 106, 0);
  calibrationColor[5] = Scalar(51, 63, 120, 0);
  calibrationColor[6] = Scalar(45, 79, 86, 0);
  calibrationColor[7] = Scalar(76, 40, 118, 0);
  calibrationColor[8] = Scalar(71, 67, 109, 0);

  debugMode=false;
  std::string shrunkImagePath("db/trainingSet");
  std::string bigImagePath("db/big/bigSet");
  parseCommandLineArgs(argc,argv);

  namedWindow("gloveTrack", 1);
  setMouseCallback("gloveTrack", mouseCallback, NULL);

  VideoCapture captureDevice(1);
  if (!captureDevice.isOpened()) {
    std::cerr << " Unable to open video capture device" << std::endl;
    exit(1);
  }

  iWidth = captureDevice.get(CV_CAP_PROP_FRAME_WIDTH);
  iHeight = captureDevice.get(CV_CAP_PROP_FRAME_HEIGHT);

  //Size of reduced dimensionality image
  int databaseImageWidth = 50;
  int databaseImageHeight = 50;


  //Load image database
  bool imagesLeftToLoad=true;
  int index = 0;
  while (imagesLeftToLoad==true) {
    std::string imageInputPath(concatStringInt(shrunkImagePath,index));
    imageInputPath.append(".jpg");
    std::cerr << "Loading into database:" << imageInputPath << std::endl;

    Mat trainImg = imread(imageInputPath,1);
    if (trainImg.data==NULL) {
      std::cerr << "Unable to read:" << imageInputPath << std::endl << "Finished reading database (or else missing file, incorrect permissions, unsupported/invalid format)" << std::endl;
    imagesLeftToLoad=false;
    } else {
      comparisonImages.push_back(trainImg.clone()); //img will already be correct size, so no need to get portion via boundbox
      index++;
    }
  }
  int initialImageDatabaseSize=index;


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
      std::cerr << "P pressed. Placing photo number " <<  numImagesTaken << " at " << numImagesTaken+initialImageDatabaseSize << std::endl;
      comparisonImages.push_back(currentFrame);//immediately make new comparison image this photo
      numImagesTaken++;
    }

    Rect calibrationRect = Rect( (iWidth/2.0), (iHeight/20.0), 25, 45); //take color from here
    if( (char)c == 'c' ) {
      std::cerr << "Calibrate" << std::endl;
      calibrate(frame, calibrationRect);
    }
    if (debugMode==true){
      //draw little square to show which color is being calibrated
      Rect selectorSymbolRect = Rect( (iWidth/2.0) - calibrationRect.width/2, (iHeight/20.0)+ calibrationIndex*calibrationRect.height +calibrationRect.height/2, 10, 10); //nicely located to the left of the calibration colors
      Mat selectorSymbol(frame,selectorSymbolRect);
      selectorSymbol = Scalar(0,0,0,0);//black square;
      selectorSymbol.copyTo(frame(selectorSymbolRect));
      
	//draw actual calibration colors on screen
      for (int i=0;i<NUMGLOVECOLORS;i++) {
	Mat smallBlockOfColor(frame, calibrationRect);
	smallBlockOfColor = calibrationColor[i];
	smallBlockOfColor.copyTo(frame(calibrationRect));
	calibrationRect.y += calibrationRect.height;
      }
    }


    if( (char)c == 'q' ) {
      if (debugMode==true) {
	for (int i=initialImageDatabaseSize;i<numImagesTaken+initialImageDatabaseSize;i++){
	  std::string shrunkImageFilepath(concatStringInt(shrunkImagePath,i));
	  shrunkImageFilepath.append(".jpg");
	  std::cerr << "Saving shrunk photos in " << shrunkImageFilepath << std::endl;
	  imwrite( shrunkImageFilepath, comparisonImages.at(i));

	  //Write out full size images
	  //std::string bigImagePath(concatStringInt(bigImagePath,i));
	  //bigImagePath.append(".jpg");
	  //std::cerr << "Saving calibration photos in " << bigImagePath << std::endl;
	  //imwrite( bigImagePath, comparisonImages.at(i));
	}


	std::cerr << "Calibration colors were: " << std::endl;
	for (int i=0;i<NUMGLOVECOLORS;i++) {
	  std::cerr << calibrationColor[i] << std::endl;
	}
	std::cerr << std::endl;
      }
      exit(0);
    }

    imshow("gloveTrack",frame);
    t = ((double)getTickCount() - t)/getTickFrequency();
    if(debugMode==true){std::cout << "Times passed in seconds: " << t << std::endl;}
  }

  return (0);
}


//Pre-C++11 standard doesn't have string class concat, atoi not standardized
std::string concatStringInt(std::string part1,int part2) {
    std::stringstream ss;
    ss << part1 << part2;
    return (ss.str());
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
