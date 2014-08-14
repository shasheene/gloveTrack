#include "lookupDatabase.h"

int queryDatabasePose(Mat curr) {
  int rows = curr.rows;
  int cols = curr.cols;

  int indexOfSmallestHamming = -1;
  int smallestHamming = curr.rows * curr.cols + 1;
  //std::cerr << smallestHamming << std::endl;

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
