#include "gloveTrack.h"

void parseCommandLineArgs(int, char**);
std::string concatStringInt(std::string part1,int part2);
Mat captureFrame(VideoCapture device);
int numImagesTaken = 0;

//Globals
bool debugMode;
std::vector<Mat> comparisonImages;
double iWidth, iHeight;
const int numGloveColors=5;
Scalar calibrationColor[numGloveColors];

int main(int argc, char** argv){
  debugMode=false;
  std::string databasePath("db/trainingSet");
  parseCommandLineArgs(argc,argv);

  Mat frame;
  
  VideoCapture captureDevice(0);
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
    std::string imageInputPath(concatStringInt(databasePath,index));
    imageInputPath.append(".jpg");
    std::cerr << "Loading into database:" << imageInputPath << std::endl;

    Mat trainImg = imread(imageInputPath,1);
    if (trainImg.data==NULL) {
      std::cerr << "Unable to read:" << imageInputPath << std::endl << "Likely finished reading database (or else missing file, incorrect permissions, unsupported/invalid format)" << std::endl;
    imagesLeftToLoad=false;
    } else {

      comparisonImages.push_back(trainImg.clone()); //img already correct size, so no need to boundbox
      index++;
    }
  }


  //Untouched background for calibration
  Mat backgroundFrame = captureFrame(captureDevice);
  while( true ) {
    double t = (double)getTickCount();

    frame = captureFrame(captureDevice);

    Rect gloveRegion = locateGlove(frame); //No actual tracking yet (returns fixed region)
    rectangle(frame, gloveRegion, Scalar(0,0,0)); //Draw rectangle represententing tracked location
    
    Mat currentFrame = frame(gloveRegion);

    //Mat shrunkFrame = reduceDimensions(currentFrame, 50, 50);
    Mat shrunkFrame = currentFrame;


    Mat backgroundRemovalFrame = backgroundFrame(gloveRegion);
    shrunkFrame = cleanupImage(shrunkFrame, backgroundRemovalFrame);

    //Draw shrunkFrame on given point on screen (later only in debug mode)
    Rect regionOfInterest(Point(30,35), shrunkFrame.size());
    shrunkFrame.copyTo(frame(regionOfInterest));
    


    if (comparisonImages.size() > 0){
      int indexOfMatch = queryDatabasePose(currentFrame);
      Rect roi(Point(100,240), comparisonImages.at(indexOfMatch).size());

      //Isolate below into "getPoseImage()" later:
      comparisonImages.at(indexOfMatch).copyTo(frame(roi));
    }


    //READ KEYBOARD
    int c = waitKey(10);
    if( (char)c == 'p' ) {
      Mat photo;
      photo = captureFrame(captureDevice);
      photo = photo(gloveRegion);
      std::string imageOutputPath(concatStringInt(databasePath,numImagesTaken));
      imageOutputPath.append(".jpg");
      std::cerr << "P pressed. Saving photo in " << imageOutputPath << std::endl;
      imwrite( imageOutputPath, photo );
      comparisonImages.push_back(photo);//immediately make new comparison image this photo
      numImagesTaken++;
    }

    Rect calibrationRect = Rect( (iWidth/2.0), (iHeight/20.0), 25, 45);
    if( (char)c == 'c' ) {
      std::cerr << "Calibrate" << std::endl;
      calibrate(frame, calibrationRect);
    }
    if (debugMode==true){
      for (int i=0;i<numGloveColors;i++) {
	rectangle(frame, calibrationRect, calibrationColor[i]); //draw rect
	calibrationRect.y += calibrationRect.height;
      }
    }


    if( (char)c == 'q' ) {
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
