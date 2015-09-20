#include "gloveTrack.hpp"

std::string concatStringInt(std::string part1, int part2);

Mat captureFrame(VideoCapture device); //takes photo and returns it
int numImagesTaken = 0;

//Globals (declared extern'd in libsAndConst.h and defined mostly in main)
std::vector<Mat> comparisonImages;
std::vector<Mat> testingImages;
double image_width, image_height;
int thresholdBrightness;


Mat frame;

Scalar classificationColor[NUMGLOVECOLORS];
int classificationArrayIndex; //used in mouse call back

Scalar blenderGloveColor[NUMGLOVECOLORS];

//Debug and helper function
bool openCaptureDevice(VideoCapture &captureDevice, int deviceNumber);
void drawCurrentClassificationColors(Mat &targetFrame); //draws vertical squares representing classifcation color
bool realTimeMode = true;
int videoCaptureDeviceNumber = 0;

bool slowMode; //extra info above debug mode

int main(int argc, char** argv) {
    //In other functions, initially get a logger reference with "spdlog::get("console")"
    auto console = spdlog::stdout_logger_mt("console");
    console->info("gloveTrack");

    struct arguments args;
    args.interactiveMode = false;
    args.videoCaptureDevice = 0;
    args.numGloveColors = 0;
    args.processingWidth = 25;
    args.processingHeight = 25;
    args.normalizedWidth = 25;
    args.normalizedHeight = 25;
    args.displayWidth = 75;
    args.displayHeight = 75;

    parseCommandLineArgs(argc, argv, args);
    std::string trainingImagePath("db/blenderImg/");
    std::string testingImagePath("db/test/");

    namedWindow("gloveTrack", 1);
    setMouseCallback("gloveTrack", mouseCallback, NULL);
    //REMEMBER OpenCV color space is BGR, not RGB
    //Actual blender cols:
    blenderGloveColor[0] = Scalar(0, 0, 0, 0); //black
    blenderGloveColor[1] = Scalar(34, 29, 180, 0); //red
    blenderGloveColor[2] = Scalar(9, 65, 2, 0); //green
    blenderGloveColor[3] = Scalar(99, 58, 35, 0); //dark blue
    blenderGloveColor[4] = Scalar(21, 138, 247, 0); //orang
    blenderGloveColor[5] = Scalar(154, 153, 67, 0); //light blue
    blenderGloveColor[6] = Scalar(137, 101, 171, 0); //purple
    blenderGloveColor[7] = Scalar(90, 106, 253, 0); //pink

    //Current glove colors - manually picked from test1.jpg camera image
    classificationColor[0] = Scalar(0, 0, 0, 0); //bkg
    classificationColor[1] = Scalar(119, 166, 194, 0); //white
    classificationColor[2] = Scalar(22, 29, 203, 0); //red
    classificationColor[3] = Scalar(32, 155, 169, 0); //green
    classificationColor[4] = Scalar(77, 13, 19, 0); //dark blue
    classificationColor[5] = Scalar(14, 95, 206, 0); //orange
    classificationColor[6] = Scalar(129, 151, 102, 0); //light blue
    classificationColor[7] = Scalar(94, 121, 208, 0); //pink
    classificationColor[8] = Scalar(88, 52, 94, 0); //purple

    //First glove colors - manually picked from test0001.png camera image
    /*  classificationColor[0] = Scalar(255, 255, 255, 0);//bkg
        classificationColor[1] = Scalar(0, 0, 0, 0);//black
        classificationColor[2] = Scalar(34, 29, 180, 0);//red
        classificationColor[3] = Scalar(9, 65, 2, 0);//green
        classificationColor[4] = Scalar(99, 58, 35, 0);//dark blue
        classificationColor[5] = Scalar(21, 138, 247, 0);//orange
        classificationColor[6] = Scalar(154, 153, 67, 0);//light blue
        classificationColor[7] = Scalar(90, 106, 253, 0);//pink
        classificationColor[8] = Scalar(137, 101, 171, 0);//purple
     */
    if (realTimeMode == false) {
        //Size of reduced dimensionality image
        int databaseImageWidth = 50;
        int databaseImageHeight = 50;

        //Load image database
        //loadImageDatabase(comparisonImages, trainingImagePath, 64); //wrong for testing
        loadCameraImageDatabase(testingImages, testingImagePath, 64);

        int no_of_clusters = NUMGLOVECOLORS;
        EM em(no_of_clusters); //Expectation Maximization Object with ... clusters.
        int resultToIndex[NUMGLOVECOLORS]; //initialized in training function

        std::cout << "Loading expectation maximization training set" << std::endl;

        Mat rawTrainingImages[1];
        Mat labelledTrainingImages[1]; //colors will be classified/calibrated either manually by coloring Photoshop/Gimp/etc, or algorithmically
        rawTrainingImages[0] = fastReduceDimensions(imread("db/trainingSet/train1_unlabelled.png", 1), 10);
        labelledTrainingImages[0] = fastReduceDimensions(imread("db/trainingSet/train1_labelled.png", 1), 10);

        console->info("Training expectation maximization model");
        trainExpectationMaximizationModel(rawTrainingImages, labelledTrainingImages, 1, em, resultToIndex); //Magic 2, the number of training images. fix

        Mat testImages[6];

        testImages[0] = imread("db/testingSet/test1.jpg", 1);
        testImages[1] = imread("db/testingSet/test2.jpg", 1);
        testImages[2] = imread("db/testingSet/test3.jpg", 1);
        testImages[3] = imread("db/testingSet/test4.png", 1);
        testImages[4] = imread("db/testingSet/test5.png", 1);
        testImages[5] = imread("db/testingSet/test6.jpg", 1);

        for (int i = 0; i < 6; i++) {
            Mat input = fastReduceDimensions(testImages[i], 50);
            SPDLOG_TRACE(console, "Running EM on query image");
            Mat normalizedImage = normalizeQueryImage(input, em, resultToIndex, args);
            imshow("gloveTrack", normalizedImage);
            waitKey(0);
        }

        for (int i = 0; i < testingImages.size(); i++) {
            console->info("Testing image number {}", i);

            imshow("gloveTrack", testingImages.at(i));
            waitKey(0);

            //Output X nearest neighbors by weighted hamming distance, 
            std::vector<int> nearestNeighboors = queryDatabasePose(testingImages.at(i));

            for (int i = 0; i < nearestNeighboors.size(); i++) {
                console->info("Testing image number {}", nearestNeighboors.at(i));
            }
            console->info("Waiting for user input before moving to next image");
            waitKey(0);
        }
    } else {
        int thresholdBrightness = 64;

        std::string databaseImagePath("db/blenderImg"); //query image path - temp

        VideoCapture captureDevice;
        //openCaptureDevice(captureDevice, videoCaptureDeviceNumber); //videoCaptureDeviceNumber from -c argument

        const char* video = "db/testingSet/fullFrameVideo.mp4";
        console->info("Attempting to open {}", video);
        captureDevice.open(video);
        if (!captureDevice.isOpened()) {
            console->info("Unable to open video capture device {} too. Quitting", videoCaptureDeviceNumber);
            exit(1);
        }

        int no_of_clusters = NUMGLOVECOLORS;
        EM em(no_of_clusters); //Expectation Maximization Object with ... clusters.
        int resultToIndex[NUMGLOVECOLORS]; //initialized in training function

        console->info("Loading expectation maximization training set", videoCaptureDeviceNumber);
        Mat rawTrainingImages[1];
        Mat labelledTrainingImages[1]; //colors classified/calibrated either manually by coloring Photoshop/Gimp/etc, or algorithmically

        int percentScaling = 10;
        rawTrainingImages[0] = fastReduceDimensions(imread("db/trainingSet/train1_unlabelled.png", 1), percentScaling);
        labelledTrainingImages[0] = fastReduceDimensions(imread("db/trainingSet/train1_labelled.png", 1), percentScaling);
        //rawTrainingImages[0] = fastReduceDimensions(imread("../db/newGlove/trainSmall1.png",1), percentScaling);
        //labelledTrainingImages[0] = fastReduceDimensions(imread("../db/newGlove/trainLabelledSmall1.png",1),percentScaling);
        //rawTrainingImages[0] = fastReduceDimensions(imread("../db/newGlove/t1.png",1), percentScaling);
        //labelledTrainingImages[0] = fastReduceDimensions(imread("../db/newGlove/t1.png",1),percentScaling);
        //rawTrainingImages[0] = fastReduceDimensions(imread("../db/newGlove/trainSmallX.png",1), percentScaling);
        //labelledTrainingImages[0] = fastReduceDimensions(imread("../db/newGlove/trainSmallXLabelled.png",1),percentScaling);

        //rawTrainingImages[1] = imread("../db/test/miniC.png",1);
        //labelledTrainingImages[1] = imread("../db/test/miniCLabelled.png",1);

        console->info("Training expectation maximization model", videoCaptureDeviceNumber);
        trainExpectationMaximizationModel(rawTrainingImages, labelledTrainingImages, 1, em, resultToIndex); //Magic 2, the number of training images. fix

        image_width = captureDevice.get(CV_CAP_PROP_FRAME_WIDTH);
        image_height = captureDevice.get(CV_CAP_PROP_FRAME_HEIGHT);

        //Size of reduced dimensionality image
        int databaseImageWidth = 50;
        int databaseImageHeight = 50;

        //Load image database
        int initialImageDatabaseSize = loadImageDatabase(comparisonImages, databaseImagePath, 64);

        while (true) {
            double t = (double) getTickCount(); //fps calculation
            frame = captureFrame(captureDevice);

            //Mat shrunkFrame = fastNormalizeQueryImage(frame, thresholdBrightness);
            Mat shrunkFrame = normalizeQueryImage(frame, em, resultToIndex, args);

            if (slowMode == true) {
                //draw on screen (later debug only)
                Rect currentFrameScreenLocation(Point(40, 40), frame.size());
                frame.copyTo(frame(currentFrameScreenLocation));
            }

            //Second run over image for faster lookup later. (May merge with cleanup)
            Rect shrunkFrameScreenLocation(Point(0, 0), shrunkFrame.size()); //Draw shrunkFrame on given point on screen (later only in debug mode)
            shrunkFrame.copyTo(frame(shrunkFrameScreenLocation));
            /*
            if (comparisonImages.size() > 0){
              std::vector<int> indexOfMatch = queryDatabasePose(currentFrame);
              Rect roi(Point(100,240), comparisonImages.at(indexOfMatch.at(0)).size());
              //Isolate below into "getPoseImage()" later:
              comparisonImages.at(indexOfMatch.at(0)).copyTo(frame(roi));
              }*/


            //drawCurrentClassificationColors(frame);


            //READ KEYBOARD
            int c = waitKey(1);
            switch (c) {
                case 'p':
                    console->info("P pressed. Pushing back photo number {} into {}", numImagesTaken, numImagesTaken + initialImageDatabaseSize);
                    //comparisonImages.push_back(currentFrame);//immediately make new comparison image this photo
                    //numImagesTaken++;
                    break;
                case '[':
                    if (thresholdBrightness > 0) {
                        thresholdBrightness--;
                    }
                    console->info("Threshold brightness now: {}", thresholdBrightness);
                    break;
                case ']':
                    if (thresholdBrightness < 255) {
                        thresholdBrightness++;
                    }
                    console->info("Threshold brightness now: {}", thresholdBrightness);
                    break;
                case 'q':
                    /*if (verbosity > 0) {
                      //Backup unsaved comparison image files:
                      saveDatabase(comparisonImages, initialImageDatabaseSize, databaseImagePath);

                      //Later, save classification colors to image file
                      for (int i = 0; i < NUMGLOVECOLORS; i++) {
                        console->info("{}", classificationColor[i]);
                      }
                    }*/
                    exit(0);
                    break;
            }

            imshow("gloveTrack", shrunkFrame);
            t = ((double) getTickCount() - t) / getTickFrequency();
            SPDLOG_TRACE(console, "Times passed in seconds {}", t);
        }
    }
    return (0);
}

//for debug:

void drawCurrentClassificationColors(Mat &frame) {
    Rect classificationRect = Rect(image_width - 25, (image_height / 20.0), 25, 45); //area to output classification colors

    //draw little square to show which color is being calibrated
    Rect selectorSymbolRect = Rect(image_width - classificationRect.width / 2 - 25, (image_height / 20.0) + classificationArrayIndex * classificationRect.height + classificationRect.height / 2, 10, 10); //nicely located to the left of the classification colors
    Mat selectorSymbol(frame, selectorSymbolRect);
    selectorSymbol = Scalar(0, 0, 0, 0); //black square;
    selectorSymbol.copyTo(frame(selectorSymbolRect));

    //draw actual classification colors on screen
    for (int i = 0; i < NUMGLOVECOLORS; i++) {
        Mat smallBlockOfColor(frame, classificationRect);
        smallBlockOfColor = classificationColor[i];
        smallBlockOfColor.copyTo(frame(classificationRect));
        classificationRect.y += classificationRect.height;
    }
}

bool openCaptureDevice(VideoCapture &captureDevice, int deviceNumber) {
    captureDevice.open(deviceNumber);
    return (captureDevice.isOpened());
}

Mat captureFrame(VideoCapture device) {
    Mat frame;
    bool readable = device.read(frame);
    if (!readable) {
        spdlog::get("console")->info("Cannot read frame from video stream");
        exit(1);
    }
    return (frame);
}
