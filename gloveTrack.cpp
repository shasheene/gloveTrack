#include "gloveTrack.hpp"

Scalar classification_color[NUMGLOVECOLORS];

bool openCaptureDevice(VideoCapture &capture_device, int device_number) {
    capture_device.open(device_number);
    return (capture_device.isOpened());
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

int main(int argc, char** argv) {
    //In other functions, initially get a logger reference with "spdlog::get("console")"
    auto console = spdlog::stdout_logger_mt("console");
    console->info("gloveTrack");

    struct arguments args;
    args.headless_mode = false;
    args.display_input_images = false;
    args.input_video_file = NULL;
    args.video_capture_device = -1;

    args.num_glove_colors = 0;

    args.pre_crop_width = 25;
    args.pre_crop_height = 25;

    args.processing_width = 25;
    args.processing_height = 25;

    args.normalized_width = 25;
    args.normalized_height = 25;

    args.display_width = 75;
    args.display_height = 75;

    args.training_set_manifest = (char*) "db/trainingSet/manifest.json";
    args.evaluation_set_manifest = (char*) "db/evaluationSet/manifest.json";
    args.search_set_manifest = (char*) "db/searchSet/manifest.json";
    args.save_normalized_images = false;
    // User specified command-line custom arguments overwrite struct's defaults
    parseCommandLineArgs(argc, argv, args);

    runMain(args);
    return 0;
}

int runMain(struct arguments args) {
    auto console = spdlog::get("console");

    Manifest trainingSetManifest = Manifest();
    trainingSetManifest.LoadManifest(args.training_set_manifest);

    Manifest evaluationSetManifest = Manifest();
    evaluationSetManifest.LoadManifest(args.evaluation_set_manifest);

    Manifest searchSetManifest = Manifest();
    searchSetManifest.LoadManifest(args.search_set_manifest);

    namedWindow("gloveTrack", WINDOW_AUTOSIZE);

    //Current glove colors - manually picked from test1.jpg camera image
    classification_color[0] = Scalar(0, 0, 0, 0); //bkg
    classification_color[1] = Scalar(119, 166, 194, 0); //white
    classification_color[2] = Scalar(22, 29, 203, 0); //red
    classification_color[3] = Scalar(32, 155, 169, 0); //green
    classification_color[4] = Scalar(77, 13, 19, 0); //dark blue
    classification_color[5] = Scalar(14, 95, 206, 0); //orange
    classification_color[6] = Scalar(129, 151, 102, 0); //light blue
    classification_color[7] = Scalar(94, 121, 208, 0); //pink
    classification_color[8] = Scalar(88, 52, 94, 0); //purple

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

    vector<Mat> raw_training_images = vector<Mat>();
    vector<Mat> labelled_training_images = vector<Mat>();

    for (int i = 0; i < train_unlabelled_set.size(); i++) {
        //colors will be classified/calibrated either manually by coloring Photoshop/Gimp/etc, or algorithmically
        raw_training_images.push_back(fastReduceDimensions(train_unlabelled_set.at(i), 10));
    }
    for (int i = 0; i < train_labelled_set.size(); i++) {
        //colors will be classified/calibrated either manually by coloring Photoshop/Gimp/etc, or algorithmically
        labelled_training_images.push_back(fastReduceDimensions(train_labelled_set.at(i), 10));
    }

    LookupDb lookup_db = LookupDb();
    lookup_db.Setup();

    GloveTrack glove_track = GloveTrack();
    glove_track.Setup(raw_training_images, labelled_training_images, classification_color, lookup_db, args);

    if ((args.input_video_file == NULL) && (args.video_capture_device == -1)) {
        // Non-live mode. That is, reading individual images via manifest files

        for (int i = 0; i < evaluationSetManifest.unnormalized_images.size(); i++) {

            SPDLOG_TRACE(console, "Running EM on query image");
            vector<int> closest_match = glove_track.GetHandPose(evaluationSetManifest.unnormalized_images.at(i));
            imshow("gloveTrack", glove_track.GetLastNormalizedImage());
            waitKey(0);

            if (args.save_normalized_images == true) {
                vector<int> param = vector<int>(CV_IMWRITE_PNG_COMPRESSION, 0);
                std::stringstream ss;
                ss << "db/playSet2/test";
                ss << i;
                ss << ".png";
                cv::imwrite(ss.str(), glove_track.GetLastNormalizedImage(), param);
            }

            console->info("Testing image number {}", i);
        }
    } else {
        // Live mode (reading from video file or webcam)
        VideoCapture capture_device;

        if (args.video_capture_device != -1) {
            console->info("Attempting to open webcam: {}", args.video_capture_device);
            openCaptureDevice(capture_device, args.video_capture_device);
        } else {
            console->info("Attempting to open video file: {}", args.input_video_file);
            capture_device.open(args.input_video_file);
        }

        if (!capture_device.isOpened()) {
            console->info("Unable to open video capture device. Quitting");
            exit(1);
        }

        while (true) {
            //fps calculation
            double t = (double) getTickCount();

            Mat frame = captureFrame(capture_device);

            vector<int> closest_match = glove_track.GetHandPose(frame);
            if (args.headless_mode == false) {
                imshow("gloveTrack", glove_track.GetLastNormalizedImage());

                //If in interactive mode, display raw input frame for display/demonstration purposes
                if (args.display_input_images == true) {
                    // Displays unprocessed input frame
                    namedWindow("input frame", WINDOW_NORMAL);
                    imshow("input frame", frame);
                }
            }

            // imshow requires use waitKey(n) to display frame for n milliseconds
            waitKey(1);

            t = ((double) getTickCount() - t) / getTickFrequency();
            SPDLOG_TRACE(console, "Times passed in seconds {}", t);
        }
    }
    return (0);
}