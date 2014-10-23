#include "lookupDatabase.h"


int loadImageDatabase(std::vector<Mat> &imageVector,std::string databaseFilepathPrefix, int thresholdBrightness){
    bool imagesLeftToLoad=true;
    int index = 1; //start from 1 for blender
    while (imagesLeftToLoad==true) {
      std::string imageInputFilepath(concatStringInt(databaseFilepathPrefix,index));
      imageInputFilepath.append(".png");//png for blender images. JPG for opencv saved img
      std::cerr << "Loading into database:" << imageInputFilepath << std::endl;
      
      Mat loadedImage = imread(imageInputFilepath,1);
      if (loadedImage.data==NULL) {
	std::cerr << "Unable to read:" << imageInputFilepath << std::endl << "Finished reading database (or else missing file, incorrect permissions, unsupported/invalid format)" << std::endl;
	imagesLeftToLoad=false;
      } else {
	imageVector.push_back(normalizeQueryImage(loadedImage, thresholdBrightness).clone()); //Assumes all saved images are correct sized/valid. Cloning because OpenCV normally just overwrites the single mem allocation for efficiency.
	index++;
      }
    }
    return index;
}

int loadCameraImageDatabase(std::vector<Mat> &imageVector,std::string databaseFilepathPrefix, int thresholdBrightness){
    bool imagesLeftToLoad=true;
    int index = 1; //start from 1 for blender
    while (imagesLeftToLoad==true) {
      std::string imageInputFilepath(concatStringInt(databaseFilepathPrefix,index));
      imageInputFilepath.append(".png");//png for blender images. JPG for opencv saved img
      std::cerr << "Loading into database:" << imageInputFilepath << std::endl;
      
      Mat loadedImage = imread(imageInputFilepath,1);
      if (loadedImage.data==NULL) {
	std::cerr << "Unable to read:" << imageInputFilepath << std::endl << "Finished reading database (or else missing file, incorrect permissions, unsupported/invalid format)" << std::endl;
	imagesLeftToLoad=false;
      } else {
	imageVector.push_back(tempNormalizeCamera(loadedImage, thresholdBrightness).clone()); //Assumes all saved images are correct sized/valid. Cloning because OpenCV normally just overwrites the single mem allocation for efficiency.
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


//TEMPORARY QUCIK AND DIRTY INSERTION SORT. O(n^2) is ok as function used in offline (ie non-realtime) verification
void addToNearestNeighbor(int euclidianDist, int indexOfCandidate ,
			     std::vector<int> &indexOfNearestNeighbor,std::vector<int> &distToNearestNeighbor) {
  std::vector<int>::iterator it = distToNearestNeighbor.begin();
  bool inserted = false;
  for (int i=0;i<distToNearestNeighbor.size();i++){
    if (euclidianDist <= distToNearestNeighbor.at(i) && inserted ==false){
      it = indexOfNearestNeighbor.begin();
      indexOfNearestNeighbor.insert(it+i, indexOfCandidate);

      it = distToNearestNeighbor.begin();
      distToNearestNeighbor.insert(it+i, euclidianDist);
      inserted = true;
    }
  }
  if (distToNearestNeighbor.size() ==0 || inserted==false ){
    distToNearestNeighbor.push_back(euclidianDist);
    indexOfNearestNeighbor.push_back(indexOfCandidate);
  }
}

std::vector<int> queryDatabasePose(Mat curr) {
  //Using the Vec3b slowed current from 15ms to 30ms, so usig pointers

  std::vector<int> distToNearestNeighbor;
  std::vector<int> indexOfNearestNeighbor;
  int darkThreshold = 180;
  for (int q=0;q<comparisonImages.size();q++) {
    int runningTotalHammingDist = 0;
    for (int i=0;i<curr.rows; ++i){ 
      uchar *current_pixel = curr.ptr<uchar>(i);
      for (int j=0;j<curr.cols*curr.channels();j=j+curr.channels()){
	int colorDelta =0;
	//calculateDistanceMetric. Over every pixel of comparison image, weighted by distnace
	for (int k=0;k<comparisonImages.at(q).rows; ++k){
	  uchar *db_pixel = comparisonImages.at(q).ptr<uchar>(k);    
	  for (int l=0;l<comparisonImages.at(q).cols*comparisonImages.at(q).channels(); l=l+3){
	    float euclidianPixelDistance =  sqrt( pow((i-k),2) + pow((j/3-l/3),2) );
	    if (euclidianPixelDistance <= 7) { //we only consider pixels within 7 pixels radius. For testing 2 pixels is good
	      //std::cerr << " Comparing to pixel (" << k << "," << l/3 << ") which has euclidian 2D distance " << euclidianPixelDistance << " " << "\n";      
	      colorDelta = sqrt( pow((current_pixel[j]-db_pixel[l]),2)
				+ pow((current_pixel[j+1]-db_pixel[l+1]),2)
			      + pow((current_pixel[j+2]-db_pixel[l+2]),2));
	      
	      if (euclidianPixelDistance==0){
		euclidianPixelDistance=1; //if same pixel, don't divide by zero but give big weight
	      }
	      colorDelta = colorDelta / euclidianPixelDistance; //and weigh the pixels lower if further
	    }
	  }
	}

	if ((current_pixel[j] + current_pixel[j+1] + current_pixel[j+2]) > darkThreshold) {
	  runningTotalHammingDist += (int) colorDelta;
	}
      }
      //std::cerr << "running total of this row #" << i << ": " << runningTotalHammingDist << " on image " << q << "\n";
    }

    addToNearestNeighbor(runningTotalHammingDist,q,indexOfNearestNeighbor, distToNearestNeighbor);
    std::cerr << "working:" << q << " distance was " << runningTotalHammingDist  <<  "\n";   

  }
  return indexOfNearestNeighbor;
}
/*
	//Find smallest color difference
	int indexOfClosestColor = -1;
	int smallestDelta = 444;//Biggest euclidian RGB distance is sqrt(255^2 + 255^2 + 255^2) + 1 = 442.673
	for (int k=0; k<NUMGLOVECOLORS; k++){
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
*/

//Relatively expensive operation, but only done ONCE per frame before lookup. (The database is processed offline with millions of images processed)
//Currently does first increases brighness/contrast, then compares each pixel's hamming distance to calibrated colors
Mat cleanupImage(Mat isolatedFrame, Mat shrunkBackgroundFrame) {
  Mat returnFrame;
  isolatedFrame.copyTo(returnFrame);
  
  int rows = returnFrame.rows;
  int cols = returnFrame.cols;
  for (int i=0;i<rows;++i){
    for (int j=0;j<cols*returnFrame.channels();j=j+returnFrame.channels()){//Columns are 3-channel
      //Currently best works when background frame is black (all zeroes), and background is near white - ie, detect foreground. Works quite well for testing. In future, add better background detection
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
    ss << part1 << std::setw(4) << std::setfill('0') << part2; //append leading zeroes so format of read in is same as Blender output png
    return (ss.str());
}




