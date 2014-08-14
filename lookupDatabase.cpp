#include "lookupDatabase.h"

int queryDatabasePose(Mat curr) {
  int rows = curr.rows;
  int cols = curr.cols;
  int channels = curr.channels();//img array of pixels with multiple colour channels

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
      for (int j=0;j<cols;++j){
	//	for (int k=0; k<channels; k++) 
	int pixelDiff = abs(current_pixel[j]-db_pixel[j]) 
	    + abs(current_pixel[j+1]-db_pixel[j+1]) 
	  + abs(current_pixel[j+2]-db_pixel[j+2]);
	if ((current_pixel[j] + current_pixel[j+1] + current_pixel[j+2]) > darkThreshold) {
	  runningTotalHammingDist += pixelDiff;
	}

	  //}
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
