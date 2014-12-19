#include "isolateGlove.h"


Mat normalizeQueryImage(Mat& unprocessedCameraFrame, EM& trainedEM, int** resultToIndex) {
  //Mat frame = unprocessedCameraFrame;
  //Shrink image because 2000x2000 from phone camera is far to big
  //frame = fastReduceDimensions(frame, 500, 500);//shrink

  //CV EM requires array of "samples" (each sample is a pixel RGB value)
  Mat sampleArray = Mat::zeros( unprocessedCameraFrame.rows * unprocessedCameraFrame.cols, 3, CV_32FC1 );
  convertToSampleArray(unprocessedCameraFrame, sampleArray);
  
  //Bilateral filter to smooth sensor noise (blur)
  //

  //Empty probability matrix for EM
  Mat prob = Mat::zeros(unprocessedCameraFrame.rows*unprocessedCameraFrame.cols,8,CV_32FC1);//single channel matrix for trainM EM for probability  preset vals
    //We will have a frame size from meanshift
    
  //    Mat classifiedImage = classifyColors(unprocessedCameraFrame, sampleArray, prob, trainedEM, resultToIndex);
  //meanshift
  //
    return(prob);
}

void convertLabelledToEMInitialTrainingProbabilityMatrix(Mat prelabelledSampleArray, Mat& prob, int numClustersInEM){
  std::cout << "Converting to probability matrix:" << std::endl << "  ";
      for (int i=0; i<prelabelledSampleArray.rows;i++){
      bool labelledPixel = false;
      for (int j=0;j<numClustersInEM;j++){
	//std::cerr << "here " << i << "," << j << std::endl;
	if ((prelabelledSampleArray.ptr<float>(i)[0] == classificationColor[j][0]) && (prelabelledSampleArray.ptr<float>(i)[1] == classificationColor[j][1]) && (prelabelledSampleArray.ptr<float>(i)[2] == classificationColor[j][2])){
	    prob.ptr<float>(i)[j]= 1;
	    labelledPixel=true;
	  }
	}
	if (labelledPixel==false) {//if pixel not labelled as calibrated classification color, consider pixel as background color
	  prob.ptr<float>(0)[0]= 1;
	}
    }
    std::cout << std::endl;
    /*
    // debug print:
       std::cout << std::endl;
       for (int i=0;i<prelabelledSampleArray.rows;i++){
	 for (int j=0;j<numClustersInEM;j++){//8 is number classification colors
	   std::cout <<  prob.ptr<float>(i)[j] << " ";
	 }
	 std::cout <<  "\n";
	 }*/
}

  


bool trainExpectationMaximizationModel(Mat trainSampleArray, Mat initialTrainingProbability, EM& em, int** resultToIndex){
  int no_of_clusters = em.get<int>("nclusters");


  /*
  Mat samples = 

  //Figure out how big giant sample vector should be:
  int numSamples;
  for (int i=0;i<sizeof(trainSampleArray)/sizeof(trainSampleArray[0]);i++){
    numSamples+=trainSampleArray[i].cols + trainSampleArray[i].rows;
  }

  //Fill vector of samples with training images
  Mat samples[numSamples];
x  Mat labelledSamples[numSamples];
  for (int i=0;i<sizeof(trainSampleArray)/sizeof(trainSampleArray[0]);i++){
    convertToSampleVector(trainSampleArray[i],samples+i);
    convertToSampleVector(trainLabelled[i],labelledSamples+i);
  }
  */
 
  if (em.isTrained()==true){
      std::cerr << "EM model already trained. Exiting!\n" << std::endl;
      exit(1);
  }

  std::cout << "Starting EM training" << std::endl;
  bool trainOutcome = em.trainM(trainSampleArray, initialTrainingProbability);

    //Maps classificationColor array to EM result number:
    for (int i=0;i<no_of_clusters;i++){
      Mat testPixel = Mat(1,1,CV_8UC3);
      testPixel.ptr<uchar>(0)[0] = (int)classificationColor[i][0];
      testPixel.ptr<uchar>(0)[1] = (int)classificationColor[i][1];
      testPixel.ptr<uchar>(0)[2] = (int)classificationColor[i][2];
      Mat testPixelVector = Mat::zeros( testPixel.rows * testPixel.cols, 3, CV_32FC1 );
      convertToSampleArray(testPixel, testPixelVector);
      std::cout << "resultToIndex stuff: " << i << std::endl;
      int result = em.predict(testPixelVector.row(0))[1];
      (*resultToIndex)[result] = i;
      std::cout << "After resultToIndex stuff: " << i << std::endl;
    }
    for (int i=0;i<no_of_clusters;i++){
      std::cout << "resultToIndex " << i << " is " << (*resultToIndex)[i] << std::endl;
    }
    std::cout << "After all resultToIndex stuff: "<< std::endl;
  return (trainOutcome);
}

void classifyColors(Mat testImage, Mat testImageSampleArray, Mat& outputArray ,EM& em, int** resultToIndex){
  if (em.isTrained()==false){
      std::cerr << "EM model not trained. Exiting!\n" << std::endl;
      exit(1);
  }
  
  int index = 0;
    for( int y = 0; y < testImage.rows; y++ ) {
        for( int x = 0; x < testImage.cols*3; x=x+3) {
            int result = em.predict(testImageSampleArray.row(index))[1];
	    index++;
            //testImage[result].at<Point3i>(y, x, 0) = testImageSampleArray.at<Point3i>(y, x, 0);
	    /*	    testImage.ptr<uchar>(y)[x] = classificationColor[result][0];
	    testImage.ptr<uchar>(y)[x+1] = classificationColor[result][1];
	    testImage.ptr<uchar>(y)[x+2] = classificationColor[result][2];*/

	    

	    /*std::cerr << "result is: " << result << " ";
	      std::cerr << "classificaiton: " << (*resultToIndex)[result] << " ";
	      std::cerr << "" << classificationColor[(*resultToIndex)[result]] << std::endl;
	      std::cerr << "" << (int)testImage.ptr<uchar>(y)[x] << "," << (int)testImage.ptr<uchar>(y)[x+1] << "," << (int)testImage.ptr<uchar>(y)[x+2] << "\n";*/

	    outputArray.ptr<uchar>(y)[x] = classificationColor[(*resultToIndex)[result]][0];
	    outputArray.ptr<uchar>(y)[x+1] = classificationColor[(*resultToIndex)[result]][1];
	    outputArray.ptr<uchar>(y)[x+2] = classificationColor[(*resultToIndex)[result]][2];
        }
    }
}

//Rasterize/Convert 2D BGR image matrix (MxN size) to a 1 dimension "sample vector" matrix, where each is a BGR pixel (so, 1x(MxN) size). This is the required format for OpenCV algorithms
Mat convertToSampleArray(Mat frame, Mat& outputSampleArray) {
  std::cout << "Started conversion" << std::endl;
  int index = 0;
  for( int y = 0; y < frame.rows; y++ ) {
    for( int x = 0; x < frame.cols*3; x=x+3 ) {
      outputSampleArray.at<Vec3f>(index)[0] = (float) frame.ptr<uchar>(y)[x];
      outputSampleArray.at<Vec3f>(index)[1] = (float) frame.ptr<uchar>(y)[x+1];
      outputSampleArray.at<Vec3f>(index)[2] = (float) frame.ptr<uchar>(y)[x+2];
      index++;
    }
  }

  std::cout << "Ended flattening" << std::endl;
  //  if (verbosity >2) {
    for (int i=0;i<frame.rows*frame.cols;i++){
      std::cout <<  outputSampleArray.at<Vec3f>(i) << "\n";
    }
    // }
  return outputSampleArray;
}
 
//Perhaps Merge cleanupImage(), locateGlove etc into this function later
Mat fastNormalizeQueryImage(Mat unprocessedCameraFrame, int thresholdBrightness) {
  Rect gloveBoundingBox = fastLocateGlove(unprocessedCameraFrame, thresholdBrightness);
  //rectangle(unprocessedCameraFrame, gloveBoundingBox, Scalar(0,0,0)); //Draw rectangle represententing tracked location
  
  Mat returnFrame = unprocessedCameraFrame(gloveBoundingBox).clone();//CROP
  returnFrame = fastReduceDimensions(returnFrame, 40, 40);//shrink
  returnFrame = fastClassifyColors(returnFrame);//classified
  
  return returnFrame;
}

//Need to merge cleanupImage(), fastLocateGlove etc into this function later
Mat tempNormalizeCamera(Mat unprocessedCameraFrame, int thresholdBrightness) {
  Rect gloveBoundingBox = fastLocateGlove(unprocessedCameraFrame, thresholdBrightness);
  //rectangle(unprocessedCameraFrame, gloveBoundingBox, Scalar(0,0,0)); //Draw rectangle represententing tracked location
  
  Mat returnFrame = unprocessedCameraFrame(gloveBoundingBox).clone();//CROP
  returnFrame = fastReduceDimensions(returnFrame, 40, 40);//shrink
  returnFrame = classifyCamera(returnFrame);//classified
  
  return returnFrame;
}

Rect fastLocateGlove(Mat region, int darkThreshold) {
  //LOCATE GLOVE (ie DETERMINE BOUNDING BOX):
  int numRows = region.rows;
  int numCols = region.cols;
  int numChannels = region.channels();

  int gloveRowStart, gloveRowEnd, gloveColStart, gloveColEnd;
  gloveColStart = numCols;//allbackwards on purpose
  gloveRowStart = numRows;
  gloveColEnd = 0;
  gloveRowEnd = 0;
  
  for (int i=0;i<numRows;i++){
    for (int j=0;j<numCols*numChannels;j=j+numChannels){//Columns are 3-channel
      if ( (region.ptr<uchar>(i)[j] > darkThreshold)
	   && (region.ptr<uchar>(i)[j+1] > darkThreshold)
	   && (region.ptr<uchar>(i)[j+2] > darkThreshold) ){//if found a reasonably good looking pixel
	/*
	  if ( sqrt(pow(region.ptr<uchar>(i)[j],2) +
	  pow(region.ptr<uchar>(i)[j+1],2) +
	  pow(region.ptr<uchar>(i)[j+2],2)) > darkThreshold ){//if found a reasonably good looking pixel
	  //later, scan nearby pixels
	  */
	
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
  
  //If something strange happend, just make tiny square
  if (gloveColEnd <= gloveColStart) {
    gloveColEnd = 100;
    gloveColStart = 0;
  }
  
  if (gloveRowEnd <= gloveRowStart) {
    gloveRowEnd = 100;
    gloveRowStart = 0;
  }
  
  return( Rect( gloveColStart, gloveRowStart, gloveColEnd-gloveColStart, gloveRowEnd-gloveRowStart) );
}

//If in future may be worth it to merge this with cleanupImage so only a single cycle over fullsize image. But current is better for code clarity
Mat fastReduceDimensions(Mat region, int targetWidth, int targetHeight) {
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


  return (shrunkFrame.clone());
}


Mat fastClassifyColors(Mat croppedImage) {
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
	  colorDeltaOfCurrentPixel[l] = croppedImage.ptr<uchar>(i)[j+l] - blenderGloveColor[k][l];
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

/*void convertToArray(Mat frame) {
  int numRows = croppedImage.rows; //should be fixed later on
  int numCols = croppedImage.cols;
  int numChannels = croppedImage.channels();

  Mat
  *array = (CV_64F) malloc(numChannels);

  
  for (int i=0;i<numRows;++i){
    for (int j=0;j<numCols*numChannels;j=j+numChannels){
    }
  }
  }*/

Mat classifyCamera(Mat croppedImage) {
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
