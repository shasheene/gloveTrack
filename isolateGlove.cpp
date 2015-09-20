#include "isolateGlove.hpp"
#include "commandLineArguments.hpp"

Mat normalizeQueryImage(Mat& unprocessedCameraFrame, EM& trainedEM,
        int (&resultToIndex)[NUMGLOVECOLORS], struct arguments args) {
    Mat frame = unprocessedCameraFrame;
    //We shrink the image because 2000x2000 from phone camera is far to big
    //frame = fastReduceDimensions(frame, 500, 500);//shrink to speedup other algorithm

    //CV EM requires array of "samples" (each sample is a pixel RGB value)
    SPDLOG_TRACE(spdlog::get("console"), "\"Flattening\" input image into array of samples (pixels) for further processing");

    Mat shrunkFrame = Mat::zeros(args.processingWidth, args.processingHeight, CV_8UC3);
    resize(frame, shrunkFrame, shrunkFrame.size(), 0, 0, INTER_LINEAR);

    //Mat sampleArray = Mat::zeros( shrunkFrame.rows * shrunkFrame.cols, 3, CV_32FC1 );
    //convertToSampleArray(shrunkFrame, sampleArray);


    //if (verbosity>0)

    SPDLOG_TRACE(spdlog::get("console"), "Bilateral filter to smooth sensor noise (by slight image bluring)");
    Mat filtered = Mat::zeros(shrunkFrame.rows, shrunkFrame.cols, CV_8UC3);
    SPDLOG_TRACE(spdlog::get("console"), "bilateral filter complete");
    bilateralFilter(shrunkFrame, filtered, 50, 5, BORDER_DEFAULT); //filtersize, sigma color
    frame = filtered;
    /*  Mat outputFrame = Mat::zeros(OUTPUT_WIDTH,OUTPUT_HEIGHT,CV_8UC3);
    resize(filtered,outputFrame,outputFrame.size(),0,0,INTER_LINEAR);
    return outputFrame;*/

    //if (verbosity>0)
    //std::cout << "Cropping image using several interations of meanshift clustering algorithm\n";

    SPDLOG_TRACE(spdlog::get("console"), "Expectation Maximization prediction on every pixel to classify the colors as either background or one of the glove colors");

    Mat sampleArray = Mat::zeros(frame.rows * frame.cols, 3, CV_32FC1);
    convertToSampleArray(frame, sampleArray);

    Mat returnFrame = Mat::zeros(frame.rows, frame.cols, CV_8UC3);
    classifyColors(frame, sampleArray, returnFrame, trainedEM, resultToIndex);

    Mat cropped = meanShiftCrop(returnFrame, 2000, 1);

    /*
    Rect gloveBoundingBox = fastLocateGlove(returnFrame, 60);
    //rectangle(returnFrame, gloveBoundingBox, Scalar(255,255,255)); //Draw rectangle represententing tracked location
    returnFrame = returnFrame(gloveBoundingBox).clone(); //CROP


    //Shrink then blow up
    Mat smallFrame = Mat::zeros(args.normalizedWidth, args.normalizedHeight, CV_8UC3);
    resize(returnFrame, smallFrame, smallFrame.size(), 0, 0, INTER_LINEAR);
    Mat outputFrame = Mat::zeros(args.displayWidth, args.displayHeight, CV_8UC3);
    resize(smallFrame, outputFrame, outputFrame.size(), 0, 0, INTER_LINEAR);
    SPDLOG_TRACE(spdlog::get("console"), "EM Classification Complete");
     */
    //returnFrame = fastReduceDimensions(returnFrame, 10);//shrink
    Mat outputFrame = Mat::zeros(args.displayWidth, args.displayHeight, CV_8UC3);
    if (cropped.cols > 10 && cropped.rows > 10) {
        resize(cropped, outputFrame, outputFrame.size(), 0, 0, INTER_LINEAR);
    }
    return (outputFrame);
}

void classifyColors(Mat testImage, Mat testImageSampleArray, Mat& outputArray, EM& em, int (&resultToIndex)[NUMGLOVECOLORS]) {
    if (em.isTrained() == false) {
        spdlog::get("console")->error("EM model not trained. Exiting!");
        exit(1);
    }

    int index = 0;
    for (int y = 0; y < testImage.rows; y++) {
        for (int x = 0; x < testImage.cols * 3; x = x + 3) {
            int result = em.predict(testImageSampleArray.row(index))[1];
            index++;
            //testImage[result].at<Point3i>(y, x, 0) = testImageSampleArray.at<Point3i>(y, x, 0);
            /*	    testImage.ptr<uchar>(y)[x] = classificationColor[result][0];
            testImage.ptr<uchar>(y)[x+1] = classificationColor[result][1];
            testImage.ptr<uchar>(y)[x+2] = classificationColor[result][2];*/



            /*std::cerr << "result is: " << result << " ";
              std::cerr << "classificaiton: " << resultToIndex[result] << " ";
              std::cerr << "" << classificationColor[resultToIndex[result]] << std::endl;
              std::cerr << "" << (int)testImage.ptr<uchar>(y)[x] << "," << (int)testImage.ptr<uchar>(y)[x+1] << "," << (int)testImage.ptr<uchar>(y)[x+2] << "\n";*/

            outputArray.ptr<uchar>(y)[x] = classificationColor[resultToIndex[result]][0];
            outputArray.ptr<uchar>(y)[x + 1] = classificationColor[resultToIndex[result]][1];
            outputArray.ptr<uchar>(y)[x + 2] = classificationColor[resultToIndex[result]][2];
        }
    }
}

bool trainExpectationMaximizationModel(Mat rawTrainingImages[], Mat labelledTrainingImages[], int numTrainingImages, EM& em, int (&resultToIndex)[NUMGLOVECOLORS]) {
    auto console = spdlog::get("console");

    int no_of_clusters = em.get<int>("nclusters");

    console->debug("Flattening set of trainingImages and labelledTraining Images into two giant sample arrays");
    //Figure out how big giant sample vector should be:
    int numSamples = 0;
    for (int i = 0; i < numTrainingImages; i++) {
        numSamples += (rawTrainingImages[i].cols * rawTrainingImages[i].rows);
    }
    console->debug("Sample arrays have length {}", numSamples);

    //Fill vector of samples with training images
    Mat samples = Mat::zeros(numSamples, 3, CV_32FC1);
    Mat labelledSamples = Mat::zeros(numSamples, 3, CV_32FC1);
    int samplesOffset = 0;
    for (int i = 0; i < numTrainingImages; i++) {
        Mat tempSamples = Mat::zeros(rawTrainingImages[0].rows * rawTrainingImages[0].cols, 3, CV_32FC1);
        Mat tempLabelledSamples = Mat::zeros(rawTrainingImages[0].rows * rawTrainingImages[0].cols, 3, CV_32FC1);
        convertToSampleArray(rawTrainingImages[i], tempSamples);
        convertToSampleArray(labelledTrainingImages[i], tempLabelledSamples);
        //Append tempSamples and tempLabelled into samples and labelledSamples respectively
        for (int j = 0; j < tempSamples.rows; j++) {
            tempSamples.row(j).copyTo(samples.row(j + samplesOffset));
            tempLabelledSamples.row(j).copyTo(labelledSamples.row(j + samplesOffset));
        }
        samplesOffset += tempSamples.rows; //increment offset
    }

    /*    convertToSampleArray(rawTrainingImages, rawTrainingImagesSamples);
      Mat labelledTrainingImagesSamples = Mat::zeros( labelledTrainingImages.rows * labelledTrainingImages.cols, 3, CV_32FC1 );
      convertToSampleArray(labelledTrainingImages, labelledTrainingImagesSamples);
     */

    Mat initialProb = Mat::zeros(labelledSamples.rows, no_of_clusters, CV_32FC1); //single channel matrix for trainM EM for probability  preset vals

    //fills prob matrix with initial probability
    convertLabelledToEMInitialTrainingProbabilityMatrix(labelledSamples, initialProb, no_of_clusters);
    //debug print probability array:
    /*   std::cout << std::endl;
       for (int i=0;i<labelledSamples.rows;i++){
         for (int j=0;j<no_of_clusters;j++){//8 is number classification colors
           std::cout <<  initialProb.ptr<float>(i)[j] << " ";
         }
         std::cout <<  "\n";
       }
     */

    if (em.isTrained() == true) {
        spdlog::get("console")->error("EM model already trained. Exiting!");
        exit(1);
    }

    console->info("Starting EM training. This may take many minutes.");
    //Important: trained with raw images, but probability generated from labelled images
    bool trainOutcome = em.trainM(samples, initialProb);

    //Initialize array to 0 (incase test cases don't cover all colors or labelled incorrectly etc)
    for (int i = 0; i < no_of_clusters; i++) {
        resultToIndex[i] = 0;
    }
    console->debug(" Creating 'resultsToIndex' mapping between EM and classificationColor calibration");
    //Maps classificationColor array to EM result number:
    for (int i = 0; i < no_of_clusters; i++) {
        Mat testPixel = Mat(1, 1, CV_8UC3);
        testPixel.ptr<uchar>(0)[0] = (int) classificationColor[i][0];
        testPixel.ptr<uchar>(0)[1] = (int) classificationColor[i][1];
        testPixel.ptr<uchar>(0)[2] = (int) classificationColor[i][2];
        Mat testPixelVector = Mat::zeros(testPixel.cols, 3, CV_32FC1);
        convertToSampleArray(testPixel, testPixelVector);
        SPDLOG_TRACE(console, "resultToIndex on classifcationColor #{}", i);
        int result = em.predict(testPixelVector.row(0))[1];
        resultToIndex[result] = i;
        console->info("Result for classificationColor #{} was {}", i, result);
    }

    for (int i = 0; i < no_of_clusters; i++) {
        console->debug("resultToIndex {} is {}", i, resultToIndex[i]);
    }
    console->debug("resultToIndex map constructed");
    return (trainOutcome);

}

void convertLabelledToEMInitialTrainingProbabilityMatrix(Mat prelabelledSampleArray, Mat& prob, int numClustersInEM) {
    auto console = spdlog::get("console");
    console->debug("Converting to probability matrix");
    for (int i = 0; i < prelabelledSampleArray.rows; i++) {
        bool labelledPixel = false;
        for (int j = 0; j < numClustersInEM; j++) {
            //std::cerr << "here " << i << "," << j << std::endl;
            if ((prelabelledSampleArray.ptr<float>(i)[0] == classificationColor[j][0])
                    && (prelabelledSampleArray.ptr<float>(i)[1] == classificationColor[j][1])
                    && (prelabelledSampleArray.ptr<float>(i)[2] == classificationColor[j][2])) {
                prob.ptr<float>(i)[j] = 1;
                labelledPixel = true;
            }
        }
        if (labelledPixel == false) {//if pixel not labelled as calibrated classification color, consider pixel as background color
            prob.ptr<float>(0)[0] = 1;
        }
    }
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


//Rasterize/Convert 2D BGR image matrix (MxN size) to a 1 dimension "sample vector" matrix, where each is a BGR pixel (so, 1x(MxN) size). This is the required format for OpenCV algorithms

Mat convertToSampleArray(Mat frame, Mat& outputSampleArray) {
    SPDLOG_TRACE(spd::get("console"), "Started conversion");
    int index = 0;
    for (int y = 0; y < frame.rows; y++) {
        for (int x = 0; x < frame.cols * 3; x = x + 3) {
            outputSampleArray.at<Vec3f>(index)[0] = (float) frame.ptr<uchar>(y)[x];
            outputSampleArray.at<Vec3f>(index)[1] = (float) frame.ptr<uchar>(y)[x + 1];
            outputSampleArray.at<Vec3f>(index)[2] = (float) frame.ptr<uchar>(y)[x + 2];
            index++;
        }
    }

    SPDLOG_TRACE(spdlog::get("console"), "Ended flattening");
    for (int i = 0; i < frame.rows * frame.cols; i++) {
        SPDLOG_TRACE(spdlog::get("console"), "{}", outputSampleArray.at<Vec3f>(i));
    }
    return outputSampleArray;
}

//Perhaps Merge cleanupImage(), locateGlove etc into this function later

Mat fastNormalizeQueryImage(Mat unprocessedCameraFrame, int thresholdBrightness) {
    Rect gloveBoundingBox = fastLocateGlove(unprocessedCameraFrame, thresholdBrightness);
    //rectangle(unprocessedCameraFrame, gloveBoundingBox, Scalar(0,0,0)); //Draw rectangle represententing tracked location

    Mat returnFrame = unprocessedCameraFrame(gloveBoundingBox).clone(); //CROP
    returnFrame = fastReduceDimensions(returnFrame, 10); //shrink
    returnFrame = fastClassifyColors(returnFrame); //classified

    return returnFrame;
}

//Need to merge cleanupImage(), fastLocateGlove etc into this function later

Mat tempNormalizeCamera(Mat unprocessedCameraFrame, int thresholdBrightness) {
    Rect gloveBoundingBox = fastLocateGlove(unprocessedCameraFrame, thresholdBrightness);
    //rectangle(unprocessedCameraFrame, gloveBoundingBox, Scalar(0,0,0)); //Draw rectangle represententing tracked location

    Mat returnFrame = unprocessedCameraFrame(gloveBoundingBox).clone(); //CROP
    returnFrame = fastReduceDimensions(returnFrame, 10); //shrink
    returnFrame = classifyCamera(returnFrame); //classified

    return returnFrame;
}

double euclidianDist(int x1, int y1, int x2, int y2) {

    return ( sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2)));

}

/**
 *  Cropping is a vital step in normalizing the image for this algorithm.
 *  We crop our frame in a reproducable manner using the meanShift algorithm.
 *
 * As in the research paper, this function uses a a uniform distribution as the 'kernel'.
 *
 *
 * @param frame
 * @param maximumIterations
 * @param minimumDistance
 * @param bandwidth - initially the size of the frame
 * @return
 */
Mat meanShiftCrop(Mat frame, int maximumIterations, int minimumDistance) {
    int cols = frame.cols;
    int rows = frame.rows;
    int channels = frame.channels();

    int currX = (int) cols / (3 * 2);
    int currY = (int) rows / 2;

    int prevX = 0;
    int prevY = 0;

    //initial bandwidth should be entire frame
    int bandwidth = rows + (cols / 3);

    int currentIter = 0;
    while ((currentIter < maximumIterations) && (euclidianDist(currX, currY, prevX, prevY) > minimumDistance)) {
        double runningTotalX = 0;
        double runningTotalY = 0;
        int numX = 0;
        int numY = 0;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols * channels; j = j + channels) {//Columns are 3-channel
                // Only count if non-black pixel
                if ((frame.ptr<uchar>(i)[j] != 0) && (frame.ptr<uchar>(i)[j + 1] != 0) && (frame.ptr<uchar>(i)[j + 2] != 0)) {
                    double euclidianDistance = euclidianDist(currX, currY, i, j);
                    // If it's within the radius
                    if (euclidianDistance < bandwidth) {
                        runningTotalX += i;
                        runningTotalY += (int) (j / channels);
                        numX++;
                        numY++;
                    }
                }
            }
        }
        prevX = currX;
        prevY = currY;
#ifdef SPDLOG_TRACE_ON
#endif
        SPDLOG_TRACE(spdlog::get("console"), "runningTotalX {}, runningTotalY {}", runningTotalX, runningTotalY);
        currX = (int) (runningTotalX / numX);
        currY = (int) (runningTotalY / numY);
        bandwidth = (int) (bandwidth * 0.99);
        currentIter++;
    }

    //Rectangle may have negative co-ordinates (hand is in top left corner)
    int topLeftX = currX - (bandwidth / 2);
    int topLeftY = currY - (bandwidth / 2);
    int bottomRightX = currX + (bandwidth / 2);
    int bottomRightY = currY + (bandwidth / 2);

    int croppedImgCols = bottomRightX - topLeftX;
    int croppedImgRows = bottomRightY - topLeftY;

    // debug parameters during fine tuning
    spdlog::get("console")->debug("{},{} and {},{}", topLeftX, topLeftY, bottomRightX, bottomRightY);
    spdlog::get("console")->debug("size {},{}", croppedImgRows, croppedImgCols);

    //quick way to align to colour channel
    while ((topLeftY % 3) != 0) {
        topLeftY++;
    }
    while ((topLeftX % 3) != 0) {
        topLeftX++;
    }

    int cropRowCount = 0;
    int cropColCount = 0;
    // Rows x columns
    Mat toReturn = Mat::zeros(croppedImgRows, croppedImgCols, CV_8UC3);
    for (int i = topLeftY; i < bottomRightY; i++) {
        for (int j = topLeftX; j < bottomRightX * channels; j = j + channels) {
            //if valid
            if ((i >= 0 && i < rows) && (j >= 0 && j < (cols * channels))) {
                for (int k = 0; k < channels; k++) {
                    toReturn.ptr<uchar>(cropRowCount)[cropColCount + k] = frame.ptr<uchar>(i)[j + k];
                }
            }
            cropColCount += channels;
        }
        cropColCount = 0;
        cropRowCount++;
    }
    return (toReturn);
}

/**
 * Initial implementation of a cropping algorithm. Simply scans horizontally
 * and vertically and determines image bounds. Doesn't check every pixel but
 * iterates a certain amount of pixels.
 *
 * Normalizing the image to be a search query in a database of thousands needs to
 * be done in a more robust manner than this. However, there is some value in a
 * fast-but-low-accuracy method for lower spec machines etc.
 *
 * @param region
 * @param darkThreshold
 * @return
 */
Rect fastLocateGlove(Mat region, int darkThreshold) {
    //LOCATE GLOVE (ie DETERMINE BOUNDING BOX):
    int numRows = region.rows;
    int numCols = region.cols;
    int numChannels = region.channels();

    int gloveRowStart, gloveRowEnd, gloveColStart, gloveColEnd;
    gloveColStart = numCols; //allbackwards on purpose
    gloveRowStart = numRows;
    gloveColEnd = 0;
    gloveRowEnd = 0;

    for (int i = 0; i < numRows; i++) {
        for (int j = 0; j < numCols * numChannels; j = j + numChannels) {//Columns are 3-channel
            if ((region.ptr<uchar>(i)[j] > darkThreshold)
                    && (region.ptr<uchar>(i)[j + 1] > darkThreshold)
                    && (region.ptr<uchar>(i)[j + 2] > darkThreshold)) {//if found a reasonably good looking pixel
                /*
                  if ( sqrt(pow(region.ptr<uchar>(i)[j],2) +
                  pow(region.ptr<uchar>(i)[j+1],2) +
                  pow(region.ptr<uchar>(i)[j+2],2)) > darkThreshold ){//if found a reasonably good looking pixel
                  //later, scan nearby pixels
                 */

                if (i < gloveRowStart) {
                    gloveRowStart = i;
                }

                if ((j / 3) < gloveColStart) {
                    gloveColStart = (j / 3);
                }
                if (i > gloveRowEnd) {
                    gloveRowEnd = i;
                }

                if ((j / 3) > gloveColEnd) {
                    gloveColEnd = (j / 3);
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

    return ( Rect(gloveColStart, gloveRowStart, gloveColEnd - gloveColStart, gloveRowEnd - gloveRowStart));
}

//If in future may be worth it to merge this with cleanupImage so only a single cycle over fullsize image. But current is better for code clarity

Mat fastReduceDimensions(Mat region, int percentScaling) {
    int targetHeight = (region.rows * percentScaling) / 100;
    int targetWidth = (region.cols * percentScaling) / 100;
    int rowSkip = region.rows / targetHeight;
    int columnSkip = region.cols / targetWidth;

    //Create new Mat large enough to hold glove image
    Mat shrunkFrame = Mat(targetHeight, targetWidth, CV_8UC3, Scalar(0, 0, 0));

    //Shrink image by merging adjacent pixels in square
    for (int i = 0; i < shrunkFrame.rows; ++i) {
        for (int j = 0; j < (shrunkFrame.cols * shrunkFrame.channels()); j = j + shrunkFrame.channels()) {

            shrunkFrame.ptr<uchar>(i)[j] = region.ptr<uchar>(i * rowSkip)[j * columnSkip];
            shrunkFrame.ptr<uchar>(i)[j + 1] = region.ptr<uchar>(i * rowSkip)[j * columnSkip + 1];
            shrunkFrame.ptr<uchar>(i)[j + 2] = region.ptr<uchar>(i * rowSkip)[j * columnSkip + 2];
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

    for (int i = 0; i < numRows; ++i) {
        for (int j = 0; j < numCols * numChannels; j = j + numChannels) {//Columns, 3 color channels - I think OpenCV is BGR (not RGB)
            //Now we have bounding box, classify colors

            //Find smallest color difference
            int indexOfClosestColor = -1;
            int smallestDelta = 444; //Biggest euclidian RGB distance is sqrt(255^2 + 255^2 + 255^2) + 1 = 442.673
            for (int k = 0; k < NUMGLOVECOLORS; k++) {
                int colorDeltaOfCurrentPixel[3]; //BGR channels
                double euclidianDistance = 0;
                for (int l = 0; l < 3; l++) {
                    colorDeltaOfCurrentPixel[l] = croppedImage.ptr<uchar>(i)[j + l] - blenderGloveColor[k][l];
                    euclidianDistance += pow(colorDeltaOfCurrentPixel[l], 2);
                }
                euclidianDistance = sqrt(euclidianDistance);
                if (smallestDelta >= (int) euclidianDistance) {
                    smallestDelta = (int) euclidianDistance;
                    indexOfClosestColor = k;
                }
            }
            if (indexOfClosestColor != 0) { //leave blank pixel if classified as background

                croppedImage.ptr<uchar>(i)[j] = classificationColor[indexOfClosestColor][0];
                croppedImage.ptr<uchar>(i)[j + 1] = classificationColor[indexOfClosestColor][1];
                croppedImage.ptr<uchar>(i)[j + 2] = classificationColor[indexOfClosestColor][2];
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

    for (int i = 0; i < numRows; ++i) {
        for (int j = 0; j < numCols * numChannels; j = j + numChannels) {//Columns, 3 color channels - I think OpenCV is BGR (not RGB)
            //Now we have bounding box, classify colors

            //Find smallest color difference
            int indexOfClosestColor = -1;
            int smallestDelta = 444; //Biggest euclidian RGB distance is sqrt(255^2 + 255^2 + 255^2) + 1 = 442.673
            for (int k = 0; k < NUMGLOVECOLORS; k++) {
                int colorDeltaOfCurrentPixel[3]; //BGR channels
                double euclidianDistance = 0;
                for (int l = 0; l < 3; l++) {
                    colorDeltaOfCurrentPixel[l] = croppedImage.ptr<uchar>(i)[j + l] - classificationColor[k][l];
                    euclidianDistance += pow(colorDeltaOfCurrentPixel[l], 2);
                }
                euclidianDistance = sqrt(euclidianDistance);
                if (smallestDelta >= (int) euclidianDistance) {
                    smallestDelta = (int) euclidianDistance;
                    indexOfClosestColor = k;
                }
            }
            if (indexOfClosestColor != 0) { //leave blank pixel if classified as background
                croppedImage.ptr<uchar>(i)[j] = classificationColor[indexOfClosestColor][0];
                croppedImage.ptr<uchar>(i)[j + 1] = classificationColor[indexOfClosestColor][1];
                croppedImage.ptr<uchar>(i)[j + 2] = classificationColor[indexOfClosestColor][2];
            }
        }
    }
    return croppedImage;
}
