#include "lookupDatabase.h"


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

//Change to this format:     Vec3b = someMat.at<Vec3b>(y,x);

//Relatively expensive operation, but only done ONCE per frame before lookup. (The database is processed offline with millions of images processed)
//Currently does first increases brighness/contrast, then compares each pixel's hamming distance to calibrated colors
Mat cleanupImage(Mat isolatedFrame, Mat shrunkBackgroundFrame) {
  Mat returnFrame;
  isolatedFrame.copyTo(returnFrame);

  int rows = returnFrame.rows;
  int cols = returnFrame.cols;
  for (int i=0;i<rows;++i){
    for (int j=0;j<cols*3;j=j+3){//Columns are 3-channel

      if ( (abs(isolatedFrame.ptr<uchar>(i)[j] - shrunkBackgroundFrame.ptr<uchar>(i)[j]) < 100)
	   && (abs(isolatedFrame.ptr<uchar>(i)[j+1] - shrunkBackgroundFrame.ptr<uchar>(i)[j+1]) < 100)
	   && (abs(isolatedFrame.ptr<uchar>(i)[j+2] - shrunkBackgroundFrame.ptr<uchar>(i)[j+2]) < 100) ){

      //INCREASE BRIGHTNESS AND CONTRAST OF PIXEL (Hopefully helpful for lookup)
      double alpha = 1.4;// double between 1.0 to 3.0
      int beta = 15;// num between 0-100
      isolatedFrame.ptr<uchar>(i)[j] = saturate_cast<uchar>(alpha*isolatedFrame.ptr<uchar>(i)[j] + beta); //newColor = alpha*colorAt(i,j) + beta
      isolatedFrame.ptr<uchar>(i)[j+1] = saturate_cast<uchar>(alpha*isolatedFrame.ptr<uchar>(i)[j+1] + beta);
      isolatedFrame.ptr<uchar>(i)[j+2] = saturate_cast<uchar>(alpha*isolatedFrame.ptr<uchar>(i)[j+2] + beta);
      

	
	//Find smallest color difference
	int indexOfClosestColor = -1;
	int smallestDelta = 255 + 255 + 255 + 1;
	for (int k=0; k< numGloveColors; k++){
	  int colorDeltaOfCurrentPixel = abs(isolatedFrame.ptr<uchar>(i)[j] - calibrationColor[k][0])
	    + abs(isolatedFrame.ptr<uchar>(i)[j+1] - calibrationColor[k][1])
	    + abs(isolatedFrame.ptr<uchar>(i)[j+2] - calibrationColor[k][2]);
	  if (smallestDelta >= colorDeltaOfCurrentPixel) {
	    smallestDelta = colorDeltaOfCurrentPixel;
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
	/*//Set pixel to "negative" style
	returnFrame.ptr<uchar>(i)[j] = isolatedFrame.ptr<uchar>(i)[j];// - shrunkBackgroundFrame.ptr<uchar>(i)[j];
	returnFrame.ptr<uchar>(i)[j+1] = isolatedFrame.ptr<uchar>(i)[j+1];// - shrunkBackgroundFrame.ptr<uchar>(i)[j+1];
	returnFrame.ptr<uchar>(i)[j+2] = isolatedFrame.ptr<uchar>(i)[j+1];// - shrunkBackgroundFrame.ptr<uchar>(i)[j+2];*/
	  
      
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
