#include "gloveTrack.h"

void parseCommandLineArgs(int, char**);
std::string concatStringInt(std::string part1,int part2);

Mat captureFrame(VideoCapture device);//takes photo and returns it
int numImagesTaken = 0;

//Globals (declared extern'd in libsAndConst.h and defined mostly in main)
int verbosity;
std::vector<Mat> comparisonImages;
std::vector<Mat> testingImages;
double iWidth, iHeight;
int thresholdBrightness;

Mat frame;

Scalar classificationColor[NUMGLOVECOLORS];
int classificationArrayIndex;//used in mouse call back

Scalar blenderGloveColor[NUMGLOVECOLORS];

//Debug and helper function
bool openCaptureDevice(VideoCapture &captureDevice, int deviceNumber);
void drawCurrentClassificationColors(Mat &targetFrame);//draws vertical squares representing classifcation color

bool realTimeMode = true;
int videoCaptureDeviceNumber = 0;

bool slowMode; //extra info above debug mode

int main(int argc, char** argv){
  verbosity=0;
  slowMode =false;
  parseCommandLineArgs(argc,argv);
  std::string trainingImagePath("db/blenderImg/");
  std::string testingImagePath("db/test/");

  namedWindow("gloveTrack", 1);
  setMouseCallback("gloveTrack", mouseCallback, NULL);
	//REMEMBER OpenCV color space is BGR, not RGB
    //Actual blender cols:
    blenderGloveColor[0] = Scalar(0, 0, 0, 0);//black
    blenderGloveColor[1] = Scalar(34, 29, 180, 0);//red
    blenderGloveColor[2] = Scalar(9, 65, 2, 0);//green
    blenderGloveColor[3] = Scalar(99, 58, 35, 0);//dark blue
    blenderGloveColor[4] = Scalar(21, 138, 247, 0);//orang
    blenderGloveColor[5] = Scalar(154, 153, 67, 0);//light blue
    blenderGloveColor[6] = Scalar(137, 101, 171, 0);//purple
    blenderGloveColor[7] = Scalar(90, 106, 253, 0);//pink

    //manually picked from test0001.png camera image
    classificationColor[0] = Scalar(255, 255, 255, 0);//bkg
    classificationColor[1] = Scalar(0, 0, 0, 0);//black
    classificationColor[2] = Scalar(34, 29, 180, 0);//red
    classificationColor[3] = Scalar(9, 65, 2, 0);//green
    classificationColor[4] = Scalar(99, 58, 35, 0);//dark blue
    classificationColor[5] = Scalar(21, 138, 247, 0);//orange
    classificationColor[6] = Scalar(154, 153, 67, 0);//light blue
    classificationColor[7] = Scalar(90, 106, 253, 0);//pink
    //classificationColor[8] = Scalar(137, 101, 171, 0);//purple

    if (realTimeMode==false){
    //Size of reduced dimensionality image
    int databaseImageWidth = 50;
    int databaseImageHeight = 50;

    //Load image database
    loadImageDatabase(comparisonImages, trainingImagePath, 64); //wrong for testing

    loadCameraImageDatabase(testingImages, testingImagePath, 64);

    //Mat rawtrainingImages[5];
    //Mat labelledtrainingImages[5];//colors classified/calibrated either manually by coloring Photoshop/Gimp/etc, or algorithmically

    Mat input = imread("db/test/mini3.png",1);
    Mat inputSamples = Mat::zeros( input.rows * input.cols, 3, CV_32FC1 );
    convertToSampleArray(input, inputSamples);
    
    Mat inputLabelled = imread("db/test/mini3Labelled.png",1);
    Mat inputLabelledSamples = Mat::zeros( inputLabelled.rows * inputLabelled.cols, 3, CV_32FC1 );
    convertToSampleArray(inputLabelled, inputLabelledSamples);

    int no_of_clusters = 8;
    EM em(no_of_clusters); //Expectation Maximization Object with ... clusters.

    int** resultToIndex; //Table to convert between EM returned values to classification colors.
      resultToIndex = new int*;
  try {
    (*resultToIndex) = new int[no_of_clusters];
  } catch (int e) {
    std::cout << "An exception occurred (perhaps new operator failed). Exception Nr. " << e << '\n';
  }
  //Clear matrix
  for (int i=0; i<no_of_clusters;i++){
    (*resultToIndex)[i] = 0;
  }


  Mat initialProb = Mat::zeros(inputLabelledSamples.rows,no_of_clusters,CV_32FC1);//single channel matrix for trainM EM for probability  preset vals

    //fills prob matrix with initial probability
    convertLabelledToEMInitialTrainingProbabilityMatrix(inputLabelledSamples, initialProb,no_of_clusters);
    //debug print probability array:
    /*   std::cout << std::endl;
       for (int i=0;i<inputLabelledSamples.rows;i++){
	 for (int j=0;j<no_of_clusters;j++){//8 is number classification colors
	   std::cout <<  initialProb.ptr<float>(i)[j] << " ";
	 }
	 std::cout <<  "\n";
       }
    */

    std::cerr << "training EM" << std::endl;
    trainExpectationMaximizationModel(inputSamples, initialProb, em,resultToIndex);
std::cout << "running EM on query image" << std::endl;
    //Mat normalizedImage = normalizeQueryImage(input, em,resultToIndex);//temp funciton signature


    Mat sampleArray = Mat::zeros( input.rows * input.cols, 3, CV_32FC1 );
    convertToSampleArray(input, sampleArray);
    Mat outputArray = Mat::zeros(input.rows,input.cols,CV_8UC3);
    classifyColors(input, sampleArray, outputArray, em, resultToIndex);
    std::cout << "EM Classification Complete\n";
    imshow("gloveTrack",outputArray);
    waitKey(0);    
    //vector<Mat> emSegment =  EMSegmentation(input, 8);
      //std::cout << "EM Classification Complete\n";
      //imshow("gloveTrack",normalizedImage);  


    //waitKey(0);

    //Mat newTest = imread("db/test/trainA.png",1);
    //Mat emSegment2 =  EMSegmentation(newTest, prob, em, 8);
    //imshow("gloveTrack",emSegment2);  
    //waitKey(0);
    std::cout << "Exiting\n";
    exit(0);
	    


    
    for (int i=0;i<testingImages.size();i++){
      std::cerr << "Testing image number " << i << std::endl;

      //imshow("gloveTrack",testingImages.at(i));  
      //waitKey(0);

      //Output X nearest neighbors by weighted hamming distance, 
      std::vector<int> nearestNeighboors = queryDatabasePose(testingImages.at(i));

      for (int i=0;i<nearestNeighboors.size();i++) {
	std::cout << nearestNeighboors.at(i) << " ";
      }

      std::cout << "\nWaiting for user input before moving  to next image " << std::endl;
      waitKey(0);
    }
  } else {
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
      Mat shrunkFrame = fastNormalizeQueryImage(frame, thresholdBrightness);

      
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
    
      if (verbosity>0){
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
	if (verbosity>0) {
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
      if(verbosity>2){std::cout << "Times passed in seconds: " << t << std::endl;}
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
	verbosity=1;//make this take debug level later
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
