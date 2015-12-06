#include "isolateGlove.hpp"
#include "commandLineArguments.hpp"

Mat normalizeQueryImage(Mat& unprocessedCameraFrame, EM& trainedEM,
        int (&resultToIndex)[NUMGLOVECOLORS], struct arguments args) {
    Mat frame = unprocessedCameraFrame;

    //A camera input of 2000x2000 needs more cycles to process, so shrink for performance closer to real-time on target systems
    Mat processingFrame = Mat::zeros(args.processingWidth, args.processingHeight, CV_8UC3);
    resize(frame, processingFrame, processingFrame.size(), 0, 0, INTER_LINEAR);
    frame = processingFrame;

    SPDLOG_TRACE(spdlog::get("console"), "Bilateral filter to smooth sensor noise (by slight image bluring)");
    Mat filtered = Mat::zeros(processingFrame.rows, processingFrame.cols, CV_8UC3);
    SPDLOG_TRACE(spdlog::get("console"), "bilateral filter complete");
    // Bilateral filter blur images slightly. Used to smooth over noise. Takes filtersize, sigma color parameters
    bilateralFilter(frame, filtered, 50, 5, BORDER_DEFAULT);
    frame = filtered;

    //CV EM requires array of "samples" (each sample is a pixel RGB value)
    SPDLOG_TRACE(spdlog::get("console"), "\"Flattening\" input image into array of samples (pixels) for further processing");
    Mat sampleArray = Mat::zeros(frame.rows * frame.cols, 3, CV_32FC1);
    convertToSampleArray(frame, sampleArray);
    Mat classifiedFrame = Mat::zeros(frame.rows, frame.cols, CV_8UC3);
    SPDLOG_TRACE(spdlog::get("console"), "Expectation Maximization prediction on every pixel to classify the colors as either background or one of the glove colors");
    classifyColors(frame, sampleArray, classifiedFrame, trainedEM, resultToIndex);
    SPDLOG_TRACE(spdlog::get("console"), "EM Classification Complete");
    frame = classifiedFrame;

    Mat preCropFrame = Mat::zeros(args.preCropWidth, args.preCropHeight, CV_8UC3);
    resize(frame, preCropFrame, preCropFrame.size(), 0, 0, INTER_LINEAR);
    Mat croppedFrame = meanShiftCrop(preCropFrame, 20, 0, args);
    frame = croppedFrame;

    //Draw rectangle represententing quick-and-dirty representation of tracked location
    //Rect gloveBoundingBox = fastLocateGlove(returnFrame, 60);
    //rectangle(returnFrame, gloveBoundingBox, Scalar(255,255,255));

    // Resize it to search query (resize will most likely shrink image)
    Mat normalizedFrame = Mat::zeros(args.normalizedWidth, args.normalizedHeight, CV_8UC3);
    resize(frame, normalizedFrame, normalizedFrame.size(), 0, 0, INTER_LINEAR);
    frame = normalizedFrame;

    //If in interactive mode, stretch image for display/demonstration purposes
    if (args.headlessMode == false) {
        Mat displayFrame = Mat::zeros(args.displayWidth, args.displayHeight, CV_8UC3);
        resize(frame, displayFrame, displayFrame.size(), 0, 0, INTER_LINEAR);
        frame = displayFrame;
    }

    return (frame);
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

std::vector<double> calcStandardDev(int populationX, int populationY, std::vector<Point> coordsOfValidPixels) {
    //auto console = spdlog::get("console");
    //console->set_level(spdlog::level::debug);

    double summationX = 0;
    double summationY = 0;
    for (int i = 0; i < coordsOfValidPixels.size(); i++) {
        summationX += pow(coordsOfValidPixels.at(i).x - populationX, 2);
        summationY += pow(coordsOfValidPixels.at(i).y - populationY, 2);
        ;
    }
    std::vector<double> toReturn;
    double divisor = coordsOfValidPixels.size() - 1;
    double xStdDev = sqrt(summationX / divisor);
    toReturn.push_back(xStdDev);
    double yStdDev = sqrt(summationY / divisor);
    toReturn.push_back(yStdDev);

    return (toReturn);
}

/* Helper function adapted from SO: 'Resize an image to a square but keep aspect ratio'
 * 
 * OpenCV API being targeted doesn't appear to be able to resize
 * while maintaining aspect ratio
 */
Mat getSquareImage(const Mat& img, int target_width) {
    int width = img.cols,
            height = img.rows;

    Mat square = Mat::zeros(target_width, target_width, img.type());

    int max_dim = (width >= height) ? width : height;
    float scale = ((float) target_width) / max_dim;
    Rect roi;
    if (width >= height) {
        roi.width = target_width;
        roi.x = 0;
        roi.height = height * scale;
        roi.y = (target_width - roi.height) / 2;
    } else {
        roi.y = 0;
        roi.height = target_width;
        roi.width = width * scale;
        roi.x = (target_width - roi.width) / 2;
    }

    resize(img, square(roi), roi.size());

    return square;
}

/**
 *  Cropping is a vital step in normalizing the image for this algorithm.
 *  We crop our frame in a reproducable manner using the meanShift algorithm.
 *
 * As in the research paper, this function uses a a uniform distribution as the 'kernel'.
 * 
 * We use a rectangle (rather say, a radius) for bandwidth.
 * Computation is much faster when not  Euclidean distance (computing squares and sqrt),
 * and for a number of reasons given the update strategy is based on the standard deviation,
 * we can update each point's x and y independently allow perfect crops even when the pixels of
 * interest are nested in a corner.
 * 
 * Once we have converged on a crop, we may have to move the pixels (nested in the corner) problem
 * to make the normalized image work
 *
 *
 * @param frame
 * @param maximumIterations
 * @param minimumDistance
 * @param bandwidth - initially the size of the frame
 * @return
 */
Mat meanShiftCrop(Mat frame, int maximumIterations, int minimumDistance, struct arguments args) {
    auto console = spdlog::get("console");
    //console->set_level(spdlog::level::debug);

    int cols = frame.cols;
    int rows = frame.rows;
    int channels = frame.channels();


    //Initially, we consider the  average (x,y) co-ordinates being the centre of the screen
    int currX = (int) cols / 2;
    int currY = (int) rows / 2;
    int prevX = 0;
    int prevY = 0;

    /* The meanshift "bandwidth" (or region of considered pixels) is initially the entire input image.
     * Every iteration, this gets progressively smaller based on the distribution of pixels within
     * the rectangle. This means the window of considered pixels cuts off misclassified pixels
     * (due to image noise) in a reasonable manner for an image that contains a single concentration
     * of classified pixels (a glove).
     * 
     * Once the meanshift algorithm has converged (found the average (x,y) co-ordinates of the pixels,
     * which corresponds to the central point glove which is the highest density within the bandwidth),
     * the bandwidthRect is the suggested crop for normalization.
     * 
     * Due to the existence of (literal) corner cases (glove in the corner of the image),
     * we initially make the bandwidth rectangle larger than the input frame by a reasonable margin,
     * (which initially means negative co-ordinates for top left corner), and then crop the resulting
     * image based on that.
     */
    const int bwInitialHeight = rows;
    const int bwInitialWidth = cols;
    const int bwInitialX = 0;
    const int bwInitialY = 0;
    Rect bandwidthRect = Rect(bwInitialX, bwInitialY, bwInitialWidth, bwInitialHeight);

    int currentIteration = 0;
    std::vector<double> standardDev;

    while ((euclidianDist(currX, currY, prevX, prevY) > minimumDistance) && (currentIteration < maximumIterations)) {
        currentIteration++;
        // We keep track of every valid pixel to calculate the standard deviation (used each iteration to update bandwidth update)
        std::vector<Point> coordsOfValidPixels;
        //console->debug("Bandwidth rectangle is {}. Current x,y is ({},{})", bandwidthRect, currX, currY);

        double runningTotalX = 0;
        double runningTotalY = 0;
        int numX = 0;
        int numY = 0;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols * channels; j = j + channels) {//Columns are 3-channel
                Point currentPixel = Point((int) (j / channels), i);

                // Only count if non-black pixel
                if ((frame.ptr<uchar>(i)[j] != 0) && (frame.ptr<uchar>(i)[j + 1] != 0) && (frame.ptr<uchar>(i)[j + 2] != 0)) {
                    // If it's within the radius
                    if (bandwidthRect.contains(currentPixel)) {
                        //console->debug("Adding pixel to {}",currentPixel);
                        runningTotalX += (int) (j / channels);
                        runningTotalY += i;
                        coordsOfValidPixels.push_back(currentPixel);
                    }
                }
            }
        }

        prevX = currX;
        prevY = currY;
        // Logging stddev calculation running total is VERY verbose
        //SPDLOG_TRACE(spdlog::get("console"), "runningTotalX {}, runningTotalY {}", runningTotalX, runningTotalY);
        currX = (int) (runningTotalX / coordsOfValidPixels.size());
        currY = (int) (runningTotalY / coordsOfValidPixels.size());

        standardDev = calcStandardDev(currX, currY, coordsOfValidPixels);
        console->debug("Standard dev is {} {}. Mean is {}, {}", standardDev.at(0), standardDev.at(1), currX, currY);
        console->debug("bandwidth rect was  {}", bandwidthRect);

        ///We update the 'bandwidth' rectangle for the next iteration of meanshift (and once converged, cropping rectangle)
        // We crop using a multiple of the standard deviation in a rectangle around the mean (exact multiplier discovered experimentally)
        float multiplier = 2;
        int x1 = currX - multiplier * standardDev.at(0);
        if (x1 <= 0) {
            x1 = 0;
        }

        int y1 = currY - multiplier * standardDev.at(1);
        if (y1 <= 0) {
            y1 = 0;
        }

        //OpenCV define Rect size using height and width, not absolute x2 and y2 values
        //We want right of mean by same amount, hence 2 stddev from top left corner.
        int width = multiplier * 2 * standardDev.at(0);
        if (width >= (args.preCropWidth - x1)) {
            width = args.preCropWidth - x1;
        }

        int height = multiplier * 2 * standardDev.at(1);
        if (height >= (args.preCropHeight - x1)) {
            height = args.preCropHeight - y1;
        }

        bandwidthRect = Rect(x1, y1, width, height);
        console->debug("Rect is {}", bandwidthRect);
    }

    // Finally create the cropped image with the discovered rectangle
    Mat croppedFrame = frame(bandwidthRect);

    // We then turn the rectangle into a square (with black vertical or horizontal borders)
    Mat returnFrame;
    if (croppedFrame.rows >= croppedFrame.cols) {
        returnFrame = getSquareImage(croppedFrame, croppedFrame.rows);
    } else {

        returnFrame = getSquareImage(croppedFrame, croppedFrame.cols);
    }

    return returnFrame;
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
