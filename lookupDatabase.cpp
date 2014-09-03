#include "lookupDatabase.h"
#include "math.h"

void setPixelBlank(Mat returnFrame, int i, int j);//temp - used in cleanup image

int queryDatabasePose(Mat curr) {
  int rows = curr.rows;
  int cols = curr.cols;
  int channels = curr.channels();//img array of pixels with multiple colour channels
  //Using the Vec3b slowed current from 15ms to 30ms, so usig pointers

  int indexOfSmallestHamming = -1;
  int smallestHamming = curr.rows * curr.cols + 1;
  //std::cerr << smallestHamming << std::endl;


  int darkThreshold = 180;
  /* My distance function is not purely Hamming, as I give disimilar colours extra higher distance  */
  for (int q=0;q<comparisonImages.size();q++) {
    int runningTotalHammingDist = 0;
    for (int i=0;i<rows; ++i){ 
      uchar *db_pixel = comparisonImages.at(q).ptr<uchar>(i);
      uchar *current_pixel = curr.ptr<uchar>(i);
      for (int j=0;j<cols;++j){//not quite correct, fix soon
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
      //Currently best works when background frame is black (all zeroes), and background is white - ie, detect foreground. Works quite well for testing
      if ( (abs(isolatedFrame.ptr<uchar>(i)[j] - shrunkBackgroundFrame.ptr<uchar>(i)[j]) < 100)
	   && (abs(isolatedFrame.ptr<uchar>(i)[j+1] - shrunkBackgroundFrame.ptr<uchar>(i)[j+1]) < 100)
	   && (abs(isolatedFrame.ptr<uchar>(i)[j+2] - shrunkBackgroundFrame.ptr<uchar>(i)[j+2]) < 100) ){
	
	increaseBrightnessAndConstrastOfPixel(isolatedFrame, i, j);
	
	//Find smallest color difference
	int indexOfClosestColor = -1;
	int smallestDelta = 444;//Biggest euclidian RGB distance is sqrt(255^2 + 255^2 + 255^2) + 1 = 442.673
	for (int k=0; k< NUMGLOVECOLORS; k++){
	  int colorDeltaOfCurrentPixel[3];//BGR channels
	  double euclidianDistance = 0;
	  for (int l=0;l<3;l++){
	    colorDeltaOfCurrentPixel[l] = isolatedFrame.ptr<uchar>(i)[j+l] - calibrationColor[k][l];
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
	returnFrame.ptr<uchar>(i)[j] = calibrationColor[indexOfClosestColor][0];
	returnFrame.ptr<uchar>(i)[j+1] = calibrationColor[indexOfClosestColor][1];
	returnFrame.ptr<uchar>(i)[j+2] = calibrationColor[indexOfClosestColor][2];
	//}
      } else {
	setPixelBlank(returnFrame,i,j);
      }
    }
  }
  return returnFrame;
}

void setPixelBlank(Mat frame,int i, int j) {
  frame.ptr<uchar>(i)[j] = 255;
  frame.ptr<uchar>(i)[j+1] = 255;
  frame.ptr<uchar>(i)[j+2] = 255;
}

void increaseBrightnessAndConstrastOfPixel(Mat frame, int row, int col) {
      //INCREASE BRIGHTNESS AND CONTRAST OF PIXEL (Hopefully helpful for lookup)
      double alpha = 1.4;// double between 1.0 to 3.0
      int beta = 15;// num between 0-100
      frame.ptr<uchar>(row)[col] = saturate_cast<uchar>(alpha*frame.ptr<uchar>(row)[col] + beta); //newColor = alpha*colorAt(row,col) + beta
      frame.ptr<uchar>(row)[col+1] = saturate_cast<uchar>(alpha*frame.ptr<uchar>(row)[col+1] + beta);
      frame.ptr<uchar>(row)[col+2] = saturate_cast<uchar>(alpha*frame.ptr<uchar>(row)[col+2] + beta);
}


