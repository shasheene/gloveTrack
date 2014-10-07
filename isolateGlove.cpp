#include "isolateGlove.h"

//Need to merge cleanupImage(), locateGlove etc into this function later
Mat normalizeQueryImage(Mat unprocessedCameraFrame) {
  Mat returnFrame;
  unprocessedCameraFrame.copyTo(returnFrame);

  //LOCATE GLOVE:
  int numRows = unprocessedCameraFrame.rows;
  int numCols = unprocessedCameraFrame.cols;
  for (int i=0;i<numRows;++i){
    for (int j=0;j<numCols*3;j=j+3){//Columns are 3-channel
      if ( (unprocessedCameraFrame.ptr<uchar>(i)[j] < 100)
	   && (unprocessedCameraFrame.ptr<uchar>(i)[j+1] < 100)
	   && (unprocessedCameraFrame.ptr<uchar>(i)[j+2] < 100) ){
	
	increaseBrightnessAndConstrastOfPixel(unprocessedCameraFrame, i, j);
	
	//Find smallest color difference
	int indexOfClosestColor = -1;
	int smallestDelta = 444;//Biggest euclidian RGB distance is sqrt(255^2 + 255^2 + 255^2) + 1 = 442.673
	for (int k=0; k< NUMGLOVECOLORS; k++){
	  int colorDeltaOfCurrentPixel[3];//BGR channels
	  double euclidianDistance = 0;
	  for (int l=0;l<3;l++){
	    colorDeltaOfCurrentPixel[l] = unprocessedCameraFrame.ptr<uchar>(i)[j+l] - classificationColor[k][l];
	    euclidianDistance += pow(colorDeltaOfCurrentPixel[l],2);
	  }
	  euclidianDistance = sqrt(euclidianDistance);
	  if (smallestDelta >= (int)euclidianDistance) {
	    smallestDelta = (int)euclidianDistance;
	    indexOfClosestColor = k;
	  }
	}
	
	
	//if (indexOfClosestColor==0) { //if clothes/skin color make pixel white
	//setPixelBlank(unprocessedCameraFrame,i,j);
	//} else {
	//Otherwise set pixel to the color determined
	unprocessedCameraFrame.ptr<uchar>(i)[j] = classificationColor[indexOfClosestColor][0];
	unprocessedCameraFrame.ptr<uchar>(i)[j+1] = classificationColor[indexOfClosestColor][1];
	unprocessedCameraFrame.ptr<uchar>(i)[j+2] = classificationColor[indexOfClosestColor][2];
	//}
      } else {
	//setPixelBlank(unprocessedCameraFrame,i,j);
      }
    }
  }
  return returnFrame;


  //CROP


  //CLASSIFY COLORS


  //RETURN PERFECT QUERTY IMAGE

}

Rect locateGlove(Mat cameraFrame) {
  Rect gloveLocation;
  //Add smarts

  gloveLocation = Rect( (iWidth/2.0), (iHeight/2.0), 200, 200);
  return gloveLocation;
}

//If in future may be worth it to merge this with cleanupImage so only a single cycle over fullsize image. But current is better for code clarity
Mat reduceDimensions(Mat region, int targetWidth, int targetHeight) {
  int horizontalSkip = region.cols / targetWidth; 
  int verticalSkip = region.rows / targetHeight;

  //Create new Mat large enough to hold glove image
  Mat shrunkFrame = region(Rect(0,0,targetWidth, targetHeight)).clone();

  int numRows = shrunkFrame.rows;
  int numChannelsInImage = shrunkFrame.channels();
  int numColumns = shrunkFrame.cols * numChannelsInImage;

  //Clear matrix to black square: (todo: Figure out OpenCV Mat constructor "type" field that creates matrix of same colour type as region given any camera input)
  for (int i=0;i< numRows; ++i){ 
    uchar *p = shrunkFrame.ptr<uchar>(i);
    for (int j=0;j<numColumns;++j){
      p[j] = 0;
    }
  }

  //Reduce colour depth reduction table (simplifies future comparisons)
  /*uchar table[256];
  for (int i=0;i< 256; ++i) {
    //table[i] = (uchar)(10 * (i/10));
    table[i] = (uchar)i; //No pixel colour change
    }*/

  //Shrink image by merging adjacent pixels in square
  for (int i=0;i< numRows; ++i){ 
    uchar *p = shrunkFrame.ptr<uchar>(i);
    for (int j=0;j<numColumns;++j){
      //p[j] = table[region.ptr<uchar>(i*horizontalSkip)[j*verticalSkip]]/(horizontalSkip*verticalSkip);
      p[j] = region.ptr<uchar>(i*horizontalSkip)[j*verticalSkip];
      //Merge adjacent pixels:
      /*for (int k=0; k<horizontalSkip; k++){
	p[j] += table[region.ptr<uchar>(i*horizontalSkip+k)[j*verticalSkip]]/(horizontalSkip*verticalSkip);
      }
      for (int l=0; l<verticalSkip; l++){
	p[j] += table[region.ptr<uchar>(i*horizontalSkip)[j*verticalSkip+l]]/(horizontalSkip*verticalSkip);
	}*/
    }
  }


  return (shrunkFrame);
}
