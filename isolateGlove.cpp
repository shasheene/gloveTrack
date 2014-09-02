#include "isolateGlove.h"

void calibrate(Mat cameraFrame, Rect calibrationRect) {
  int samplePixelYPos = calibrationRect.y + (int)(calibrationRect.height/2);
  int samplePixelXPos = calibrationRect.x + (int)(calibrationRect.width/2);

  for (int i=0;i<numGloveColors;i++) {
    Vec3b pixelColor = cameraFrame.at<Vec3b>(samplePixelYPos,samplePixelXPos); //Yes, y then x.
    calibrationColor[i] = Scalar(pixelColor[0],pixelColor[1],pixelColor[2]);
    samplePixelYPos += calibrationRect.y; //Next sampling point is the next rectangle
 }

}

Rect locateGlove(Mat cameraFrame) {
  Rect gloveLocation;
  //Add smarts

  gloveLocation = Rect( (iWidth/2.0), (iHeight/2.0), 200, 200);
  return gloveLocation;
}


Mat reduceDimensions(Mat region, int targetWidth, int targetHeight) {
  int horizontalSkip = region.cols / targetWidth; 
  int verticalSkip = region.rows / targetHeight;

  //Create new Mat large enough to hold glove image
  Mat shrunkFrame = region(Rect(0,0,targetWidth, targetHeight)).clone();

  int numRows = shrunkFrame.rows;
  int numChannelsInImage = shrunkFrame.channels();
  int numColumns = shrunkFrame.cols * numChannelsInImage;

  //Clear matrix to black sqaure: (todo: Figure out OpenCV Mat constructor "type" field that creates matrix of same colour type as region given any camera input)
  for (int i=0;i< numRows; ++i){ 
    uchar *p = shrunkFrame.ptr<uchar>(i);
    for (int j=0;j<numColumns;++j){
      p[j] = 0;
    }
  }

  //Reduce colour depth reduction table (simplifies future comparisons)
  uchar table[256];
  for (int i=0;i< 256; ++i) {
    //table[i] = (uchar)(10 * (i/10));
    table[i] = (uchar)i; //No pixel colour change
  }

  //Shrink image by merging adjacent pixels in square
  for (int i=0;i< numRows; ++i){ 
    uchar *p = shrunkFrame.ptr<uchar>(i);
    for (int j=0;j<numColumns;++j){
      p[j] = table[region.ptr<uchar>(i*horizontalSkip)[j*verticalSkip]]/(horizontalSkip*verticalSkip);
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
