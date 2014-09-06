#include "lookupDatabase.h"
#include "math.h"

int loadImageDatabase(std::vector<Mat> &imageVector,std::string databaseFilepathPrefix){
    bool imagesLeftToLoad=true;
    int index = 0;
    while (imagesLeftToLoad==true) {
      std::string imageInputFilepath(concatStringInt(databaseFilepathPrefix,index));
      imageInputFilepath.append(".jpg");
      std::cerr << "Loading into database:" << imageInputFilepath << std::endl;
      
      Mat loadedImage = imread(imageInputFilepath,1);
      if (loadedImage.data==NULL) {
	std::cerr << "Unable to read:" << imageInputFilepath << std::endl << "Finished reading database (or else missing file, incorrect permissions, unsupported/invalid format)" << std::endl;
	imagesLeftToLoad=false;
      } else {
	imageVector.push_back(loadedImage.clone()); //Assumes all saved images are correct sized/valid. Cloning because OpenCV normally just overwrites the single mem allocation for efficiency.
	index++;
      }
    }
    return index;
}

void saveDatabase(std::vector<Mat> imageVector, int originalDatabaseSize, std::string databaseFilepathPrefix){ 
  for (int i=originalDatabaseSize;i<imageVector.size();i++){
    std::string imageOutputFilepath(concatStringInt(databaseFilepathPrefix,i));
    imageOutputFilepath.append(".jpg");
    std::cerr << "Saving photos in " << imageOutputFilepath << std::endl;
    imwrite( imageOutputFilepath, comparisonImages.at(i));
  }
}


int queryDatabasePose(Mat curr) {
  int rows = curr.rows;
  int cols = curr.cols;
  int channels = curr.channels();//img array of pixels with multiple colour channels
  //Using the Vec3b slowed current from 15ms to 30ms, so usig pointers

  int indexOfSmallestHamming = -1;
  int smallestHamming = curr.rows * curr.cols + 1;
  //std::cerr << smallestHamming << std::endl;


  int darkThreshold = 180;
  //Hamming distances of Euclidian distance function (slow but accurate):
  for (int q=0;q<comparisonImages.size();q++) {
    int runningTotalHammingDist = 0;
    for (int i=0;i<rows; ++i){ 
      uchar *db_pixel = comparisonImages.at(q).ptr<uchar>(i);
      uchar *current_pixel = curr.ptr<uchar>(i);
      for (int j=0;j<cols;++j){
	int pixelDiff = abs(current_pixel[j]-db_pixel[j]) 
	    + abs(current_pixel[j+1]-db_pixel[j+1]) 
	  + abs(current_pixel[j+2]-db_pixel[j+2]);
	if ((current_pixel[j] + current_pixel[j+1] + current_pixel[j+2]) > darkThreshold) {
	  runningTotalHammingDist += pixelDiff;
	}
      }
    }
    //Determine lowest
    if (runningTotalHammingDist < smallestHamming) {
      smallestHamming = runningTotalHammingDist;
      indexOfSmallestHamming = q;
    }
  }
  
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

//Relatively expensive operation, but only done ONCE per frame before lookup. (The database is processed offline with millions of images processed)
//Currently does first increases brighness/contrast, then compares each pixel's hamming distance to calibrated colors
Mat cleanupImage(Mat isolatedFrame, Mat shrunkBackgroundFrame) {
  Mat returnFrame;
  isolatedFrame.copyTo(returnFrame);
  
  int rows = returnFrame.rows;
  int cols = returnFrame.cols;
  for (int i=0;i<rows;++i){
    for (int j=0;j<cols*3;j=j+3){//Columns are 3-channel
      //Currently best works when background frame is black (all zeroes), and background is near white - ie, detect foreground. Works quite well for testing. In future, add better background detection
      /*if ( (abs(isolatedFrame.ptr<uchar>(i)[j] - shrunkBackgroundFrame.ptr<uchar>(i)[j]) < 100)
         && (abs(isolatedFrame.ptr<uchar>(i)[j+1] - shrunkBackgroundFrame.ptr<uchar>(i)[j+1]) < 100)
         && (abs(isolatedFrame.ptr<uchar>(i)[j+2] - shrunkBackgroundFrame.ptr<uchar>(i)[j+2]) < 100) ){*/

      if ( (isolatedFrame.ptr<uchar>(i)[j] < 100)
	   && (isolatedFrame.ptr<uchar>(i)[j+1] < 100)
	   && (isolatedFrame.ptr<uchar>(i)[j+2] < 100) ){
	
	increaseBrightnessAndConstrastOfPixel(isolatedFrame, i, j);
	
	//Find smallest color difference
	int indexOfClosestColor = -1;
	int smallestDelta = 444;//Biggest euclidian RGB distance is sqrt(255^2 + 255^2 + 255^2) + 1 = 442.673
	for (int k=0; k< NUMGLOVECOLORS; k++){
	  int colorDeltaOfCurrentPixel[3];//BGR channels
	  double euclidianDistance = 0;
	  for (int l=0;l<3;l++){
	    colorDeltaOfCurrentPixel[l] = isolatedFrame.ptr<uchar>(i)[j+l] - classificationColor[k][l];
	    euclidianDistance += pow(colorDeltaOfCurrentPixel[l],2);
	  }
	  euclidianDistance = sqrt(euclidianDistance);
	  if (smallestDelta >= (int)euclidianDistance) {
	    smallestDelta = (int)euclidianDistance;
	    indexOfClosestColor = k;
	  }
	}
	
	
	//if (indexOfClosestColor==0) { //if clothes/skin color make pixel white
	//setPixelBlank(returnFrame,i,j);
	//} else {
	//Otherwise set pixel to the color determined
	returnFrame.ptr<uchar>(i)[j] = classificationColor[indexOfClosestColor][0];
	returnFrame.ptr<uchar>(i)[j+1] = classificationColor[indexOfClosestColor][1];
	returnFrame.ptr<uchar>(i)[j+2] = classificationColor[indexOfClosestColor][2];
	//}
      } else {
	setPixelBlank(returnFrame,i,j);
      }
    }
  }
  return returnFrame;
}

			    //PRIVATE HELPER FUNCTIONS:			    

void setPixelBlank(Mat frame,int i, int j) {
  frame.ptr<uchar>(i)[j] = 255;
  frame.ptr<uchar>(i)[j+1] = 255;
  frame.ptr<uchar>(i)[j+2] = 255;
}

void increaseBrightnessAndConstrastOfPixel(Mat frame, int row, int col) {
      //INCREASE BRIGHTNESS AND CONTRAST OF PIXEL (Hopefully helpful for lookup). ALPHA/BETA are #define in libAndConst file

      frame.ptr<uchar>(row)[col] = saturate_cast<uchar>(ALPHA*frame.ptr<uchar>(row)[col] + BETA); //newColor = ALPHA*colorAt(row,col) + BETA
      frame.ptr<uchar>(row)[col+1] = saturate_cast<uchar>(ALPHA*frame.ptr<uchar>(row)[col+1] + BETA);
      frame.ptr<uchar>(row)[col+2] = saturate_cast<uchar>(ALPHA*frame.ptr<uchar>(row)[col+2] + BETA);
}

std::string concatStringInt(std::string part1,int part2) {
    std::stringstream ss;
    ss << part1 << part2;
    return (ss.str());
}




