#include "gloveTrack.hpp"

std::string concatStringInt(std::string part1, int part2);

Mat captureFrame(VideoCapture device); //takes photo and returns it
int numImagesTaken = 0;

int thresholdBrightness;

Mat frame;

Scalar classificationColor[NUMGLOVECOLORS];
int classificationArrayIndex; //used in mouse call back

Scalar blenderGloveColor[NUMGLOVECOLORS];

//Debug and helper function
bool openCaptureDevice(VideoCapture &captureDevice, int deviceNumber);
void drawCurrentClassificationColors(Mat &targetFrame, int image_width, int image_height); //draws vertical squares representing classifcation color

int main(int argc, char** argv) {
    //In other functions, initially get a logger reference with "spdlog::get("console")"
    auto console = spdlog::stdout_logger_mt("console");
    console->info("gloveTrack");

    struct arguments args;
    args.headlessMode = false;
    args.displayInputImages = false;
    args.inputVideoFile = NULL;
    args.videoCaptureDevice = -1;

    args.numGloveColors = 0;

    args.preCropWidth = 25;
    args.preCropHeight = 25;

    args.processingWidth = 25;
    args.processingHeight = 25;

    args.normalizedWidth = 25;
    args.normalizedHeight = 25;

    args.displayWidth = 75;
    args.displayHeight = 75;

    args.trainingSetManifest = (char*) "db/trainingSet/manifest.json";
    args.evaluationSetManifest = (char*) "db/evaluationSet/manifest.json";
    args.searchSetManifest = (char*) "db/searchSet/manifest.json";
    args.saveNormalizedImages = false;
    // User specified command-line custom arguments overwrite struct's defaults
    parseCommandLineArgs(argc, argv, args);

    runMain(args);
    return 0;
}

int runMain(struct arguments args) {
    auto console = spdlog::get("console");

    manifest trainingSetManifest = manifest();
    trainingSetManifest.load_manifest(args.trainingSetManifest);

    manifest evaluationSetManifest = manifest();
    evaluationSetManifest.load_manifest(args.evaluationSetManifest);

    manifest searchSetManifest = manifest();
    searchSetManifest.load_manifest(args.searchSetManifest);
    
    std::string trainingImagePath("db/blenderImg/");
    std::string testingImagePath("db/test/");

    namedWindow("gloveTrack", WINDOW_AUTOSIZE);
    setMouseCallback("gloveTrack", mouseCallback, NULL);

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

    int no_of_clusters = NUMGLOVECOLORS;
    EM em(no_of_clusters); //Expectation Maximization Object with ... clusters.
    int resultToIndex[NUMGLOVECOLORS]; //initialized in training function

    std::cout << "Loading expectation maximization training set" << std::endl;
    std::vector<Mat> train_unlabelled_set = trainingSetManifest.unnormalized_images;
    std::vector<Mat> train_labelled_set = trainingSetManifest.labelled_images;
    if (train_unlabelled_set.size() != train_labelled_set.size()) {
        console->warn("Training set unnormalized_images is not the same size as labelled images! Potentially malformed manifest");
    }
    if (train_unlabelled_set.size() == 0) {
        console->error("Training set empty. Potentially malformed manifest. Exiting");
        exit(0);
    }

    Mat* rawTrainingImages;
    rawTrainingImages = new Mat[train_unlabelled_set.size()];
    Mat* labelledTrainingImages;
    labelledTrainingImages = new Mat[train_labelled_set.size()];

    for (int i = 0; i < train_unlabelled_set.size(); i++) {
        //colors will be classified/calibrated either manually by coloring Photoshop/Gimp/etc, or algorithmically
        rawTrainingImages[i] = fastReduceDimensions(train_unlabelled_set.at(i), 10);
    }
    for (int i = 0; i < train_labelled_set.size(); i++) {
        //colors will be classified/calibrated either manually by coloring Photoshop/Gimp/etc, or algorithmically
        labelledTrainingImages[i] = fastReduceDimensions(train_labelled_set.at(i), 10);
    }

    console->info("Training expectation maximization model");
    trainExpectationMaximizationModel(rawTrainingImages, labelledTrainingImages, 1, em, resultToIndex); //Magic 2, the number of training images. fix
    //free(rawTrainingImages);
    //free(labelledTrainingImages);

    if ((args.inputVideoFile == NULL) && (args.videoCaptureDevice == -1)) {
        //Size of reduced dimensionality image
        int databaseImageWidth = 50;
        int databaseImageHeight = 50;

        for (int i = 0; i < evaluationSetManifest.unnormalized_images.size(); i++) {
            //Mat input = fastReduceDimensions(evaluationSetManifest.unnormalized_images.at(i), 50);
            SPDLOG_TRACE(console, "Running EM on query image");
            Mat normalizedImage = normalizeQueryImage(evaluationSetManifest.unnormalized_images.at(i), em, resultToIndex, args);
            imshow("gloveTrack", normalizedImage);
            if (args.saveNormalizedImages==true) {
                vector<int> param = vector<int>(CV_IMWRITE_PNG_COMPRESSION,0);
                std::stringstream ss;
                ss << "db/playSet2/test";
                ss << i ;
                ss << ".png";
                cv::imwrite(ss.str(),normalizedImage,param);
            }

            console->info("Testing image number {}", i);

            //Output X nearest neighbors by weighted hamming distance, 
            std::vector<int> nearestNeighboors = queryDatabasePose(normalizedImage, searchSetManifest.unnormalized_images);

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


        if (args.videoCaptureDevice != -1) {
            console->info("Attempting to open webcam: {}", args.videoCaptureDevice);
            openCaptureDevice(captureDevice, args.videoCaptureDevice);
        } else {
            console->info("Attempting to open video file: {}", args.inputVideoFile);
            captureDevice.open(args.inputVideoFile);
        }

        if (!captureDevice.isOpened()) {
            console->info("Unable to open video capture device. Quitting");
            exit(1);
        }

        int image_width = captureDevice.get(CV_CAP_PROP_FRAME_WIDTH);
        int image_height = captureDevice.get(CV_CAP_PROP_FRAME_HEIGHT);

        //Size of reduced dimensionality image
        int databaseImageWidth = 50;
        int databaseImageHeight = 50;

        while (true) {
            double t = (double) getTickCount(); //fps calculation
            frame = captureFrame(captureDevice);

            //Mat shrunkFrame = fastNormalizeQueryImage(frame, thresholdBrightness);
            Mat shrunkFrame = normalizeQueryImage(frame, em, resultToIndex, args).clone();
            /*
                        if (slowMode == true) {
                            //draw on screen (later debug only)
                            Rect currentFrameScreenLocation(Point(40, 40), frame.size());
                            frame.copyTo(frame(currentFrameScreenLocation));
                        }
             */
            /*
            if (comparisonImages.size() > 0){
              std::vector<int> indexOfMatch = queryDatabasePose(currentFrame);
              Rect roi(Point(100,240), comparisonImages.at(indexOfMatch.at(0)).size());
              //Isolate below into "getPoseImage()" later:
              comparisonImages.at(indexOfMatch.at(0)).copyTo(frame(roi));
              }*/


            //drawCurrentClassificationColors(frame, image_width, image_height);


            //READ KEYBOARD
            int c = waitKey(1);
            switch (c) {
                case 'p':
                    //console->info("P pressed. Pushing back photo number {} into {}", numImagesTaken, numImagesTaken + initialImageDatabaseSize);
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

            if (args.headlessMode == false) {
                imshow("gloveTrack", shrunkFrame);

                //If in interactive mode, display raw input frame for display/demonstration purposes
                if (args.displayInputImages == true) {
                    // Displays unprocessed input frame
                    namedWindow("input frame", WINDOW_NORMAL);
                    imshow("input frame", frame);
                }
            }
            t = ((double) getTickCount() - t) / getTickFrequency();
            SPDLOG_TRACE(console, "Times passed in seconds {}", t);
        }
    }
    return (0);
}

//for debug:

void drawCurrentClassificationColors(Mat &frame, int image_width, int image_height) {
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
