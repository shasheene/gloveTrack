#include "isolateGlove.h"

//Need to merge cleanupImage(), locateGlove etc into this function later
Mat normalizeQueryImage(Mat unprocessedCameraFrame) {
  Mat returnFrame = unprocessedCameraFrame.clone();

  //LOCATE GLOVE (ie DETERMINE BOUNDING BOX):
  int numRows = unprocessedCameraFrame.rows;
  int numCols = unprocessedCameraFrame.cols;
  int numChannels = unprocessedCameraFrame.channels();

  int gloveRowStart, gloveRowEnd, gloveColStart, gloveColEnd;
  gloveColStart = numCols;//allbackwards on purpose
  gloveRowStart = numRows;
  gloveColEnd = 0;
  gloveRowEnd = 0;

  int darkThreshold = 64;//64 old threshold
  for (int i=0;i<numRows;i++){
    for (int j=0;j<numCols*numChannels;j=j+numChannels){//Columns are 3-channel
      if ( (unprocessedCameraFrame.ptr<uchar>(i)[j] > darkThreshold)
	   && (unprocessedCameraFrame.ptr<uchar>(i)[j+1] > darkThreshold)
	   && (unprocessedCameraFrame.ptr<uchar>(i)[j+2] > darkThreshold) ){//if found a reasonably good looking pixel
	//later, scan nearby pixels
	
	if (i < gloveRowStart) {
	  gloveRowStart = i;
	}
	
	if ((j/3) < gloveColStart) {
	  gloveColStart = (j/3);
	}
	if (i > gloveRowEnd) {
	  gloveRowEnd = i;
	}
	
	if ((j/3) > gloveColEnd) {
	  gloveColEnd = (j/3);
	}
      }
    }
  }
  std::cerr << "In (x,y): Start: (" << gloveColStart << "," << gloveRowStart <<  ") . End: (" << gloveColEnd << "," << gloveRowEnd << ")" << std::endl;      
  Rect gloveLocation = Rect( gloveColStart, gloveRowStart, gloveColEnd-gloveColStart, gloveRowEnd-gloveRowStart);
  //returnFrame = (unprocessedCameraFrame(gloveLocation)).clone();//CROP
  returnFrame = reduceDimensions(returnFrame, 40, 40);//shrink
  returnFrame = classifyColors(returnFrame);//classified
  
  return returnFrame;
}

Rect locateGlove(Mat cameraFrame) {
  Rect gloveLocation;
  //Add smarts

  gloveLocation = Rect( (iWidth/2.0), (iHeight/2.0), 200, 200);
  return gloveLocation;
}

//If in future may be worth it to merge this with cleanupImage so only a single cycle over fullsize image. But current is better for code clarity
Mat reduceDimensions(Mat region, int targetWidth, int targetHeight) {
  int columnSkip = region.cols / targetWidth; 
  int rowSkip = region.rows / targetHeight;

  //Create new Mat large enough to hold glove image
  Mat shrunkFrame = Mat(targetWidth, targetHeight, CV_8UC3, Scalar(0,0,0));

  //Shrink image by merging adjacent pixels in square
  for (int i=0;i< shrunkFrame.rows; ++i){
    for (int j=0;j<(shrunkFrame.cols*shrunkFrame.channels());j=j+shrunkFrame.channels()){
     shrunkFrame.ptr<uchar>(i)[j] = region.ptr<uchar>(i*rowSkip)[j*columnSkip];
     shrunkFrame.ptr<uchar>(i)[j+1] = region.ptr<uchar>(i*rowSkip)[j*columnSkip+1];
     shrunkFrame.ptr<uchar>(i)[j+2] = region.ptr<uchar>(i*rowSkip)[j*columnSkip+2];
      //Merge adjacent pixels:
      /*for (int k=0; k<columnSkip; k++){
	p[j] += table[region.ptr<uchar>(i*columnSkip+k)[j*rowSkip]]/(columnSkip*rowSkip);
      }
      for (int l=0; l<rowSkip; l++){
	p[j] += table[region.ptr<uchar>(i*columnSkip)[j*rowSkip+l]]/(columnSkip*rowSkip);
      }*/
    }
  }


  return (shrunkFrame);
}


Mat classifyColors(Mat croppedImage) {
  int numRows = croppedImage.rows; //should be fixed later on
  int numCols = croppedImage.cols;
  int numChannels = croppedImage.channels();

  for (int i=0;i<numRows;++i){
    for (int j=0;j<numCols*numChannels;j=j+numChannels){//Columns, 3 color channels - I think OpenCV is BGR (not RGB) 
      //Now we have bounding box, classify colors
      	
      //Find smallest color difference
      int indexOfClosestColor = -1;
      int smallestDelta = 444;//Biggest euclidian RGB distance is sqrt(255^2 + 255^2 + 255^2) + 1 = 442.673
      for (int k=0; k< NUMGLOVECOLORS; k++){
	int colorDeltaOfCurrentPixel[3];//BGR channels
	double euclidianDistance = 0;
	for (int l=0;l<3;l++){
	  colorDeltaOfCurrentPixel[l] = croppedImage.ptr<uchar>(i)[j+l] - classificationColor[k][l];
	  euclidianDistance += pow(colorDeltaOfCurrentPixel[l],2);
	}
	euclidianDistance = sqrt(euclidianDistance);
	if (smallestDelta >= (int)euclidianDistance) {
	  smallestDelta = (int)euclidianDistance;
	  indexOfClosestColor = k;
	}
      }
      if (indexOfClosestColor!=0) { //leave blank pixel if classified as background
	croppedImage.ptr<uchar>(i)[j] = classificationColor[indexOfClosestColor][0];
	croppedImage.ptr<uchar>(i)[j+1] = classificationColor[indexOfClosestColor][1];
	croppedImage.ptr<uchar>(i)[j+2] = classificationColor[indexOfClosestColor][2];
      }
    }
  }
  return croppedImage;
}
