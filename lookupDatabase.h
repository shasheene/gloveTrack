#ifndef LOOKUPDATABASE_H
#define LOOKUPDATABASE_H

#include "libsAndConst.h"

int loadImageDatabase(std::vector<Mat>& imageVector,std::string databaseFilepathPrefix); //Reads in images 0 to n at filepath $(databaseDirectoryPath)n.jpg, places into imageVector and returns n. (Note the imageVector is being passed by reference!)

void saveDatabase(std::vector<Mat> imageVector, int originalDatabaseSize, std::string databaseFilepathPrefix);


/*
Potential future public API:
int* getPostRawData();// returns int array of finger/hand/etc position for another program
Mat getPoseRawImage(Mat isolatedFrame);
int getPose3DModel();//return index to 3D model DB
*/

//If public API exist, these will be private functions:
std::vector<int> queryDatabasePose(Mat isolatedFrame);
void increaseBrightnessAndConstrastOfPixel(Mat frame, int row, int col);

//Takes webcam image and background image and returns image ready for lookup 
Mat cleanupImage(Mat isolatedFrame, Mat isolatedBackgroundFrame);//same size images

//------PRIVATE HELPER FUNCTIONS---------
//(perhaps make private member functions in future if using OO):

void setPixelBlank(Mat returnFrame, int i, int j);//Used in cleanup image. May be removed in future
std::string concatStringInt(std::string part1,int part2);//Pre-C++11 standard doesn't have string class concat, atoi not standardized
			    
			    


#endif
