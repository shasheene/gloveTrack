#include "isolateGlove.hpp"
#include "commandLineArguments.hpp"

GloveTrack::GloveTrack() {
    
}

void GloveTrack::Setup(vector<Mat> unlabelled_training_images, vector<Mat> labelled_training_images, Scalar glove_colors[NUMGLOVECOLORS], LookupDb lookup_db, struct arguments arg_struct) {
    auto console = spdlog::get("console");
    
    int no_of_clusters = NUMGLOVECOLORS;
    em = EM(no_of_clusters); //Expectation Maximization Object with given number of clusters.
    
    this->lookup_db = lookup_db;
    // Copy struct, strictly incorrect atm because contains char* internally
    memcpy(&args, &arg_struct, sizeof(arg_struct));

    console->info("Training expectation maximization model");
    TrainExpectationMaximizationModel(unlabelled_training_images, labelled_training_images); //Magic 2, the number of training images. fix
}

vector<int> GloveTrack::GetHandPose(Mat unnormalized_image) {
    last_normalized_image = Normalize(unnormalized_image);
    
    return lookup_db.EstimateHandPose(last_normalized_image); 
}

Mat GloveTrack::GetLastNormalizedImage() {
    return last_normalized_image;
}

Mat GloveTrack::Normalize(Mat unnormalized_image) {
    Mat frame = unnormalized_image;
    //A camera input of 2000x2000 needs more cycles to process, so shrink for performance closer to real-time on target systems
    Mat processing_frame = Mat::zeros(args.processing_width, args.processing_height, CV_8UC3);
    resize(frame, processing_frame, processing_frame.size(), 0, 0, INTER_LINEAR);
    frame = processing_frame;

    SPDLOG_TRACE(spdlog::get("console"), "Bilateral filter to smooth sensor noise (by slight image bluring)");
    Mat filtered = Mat::zeros(processing_frame.rows, processing_frame.cols, CV_8UC3);
    SPDLOG_TRACE(spdlog::get("console"), "bilateral filter complete");
    // Bilateral filter blur images slightly. Used to smooth over noise. Takes filtersize, sigma color parameters
    bilateralFilter(frame, filtered, 50, 5, BORDER_DEFAULT);
    frame = filtered;

    //CV EM requires array of "samples" (each sample is a pixel RGB value)
    SPDLOG_TRACE(spdlog::get("console"), "\"Flattening\" input image into array of samples (pixels) for further processing");
    Mat sample_array = Mat::zeros(frame.rows * frame.cols, 3, CV_32FC1);
    ConvertToSampleArray(frame, sample_array);
    Mat classified_frame = Mat::zeros(frame.rows, frame.cols, CV_8UC3);
    SPDLOG_TRACE(spdlog::get("console"), "Expectation Maximization prediction on every pixel to classify the colors as either background or one of the glove colors");
    ClassifyColors(frame, sample_array, classified_frame);
    SPDLOG_TRACE(spdlog::get("console"), "EM Classification Complete");
    frame = classified_frame;

    Mat pre_crop_frame = Mat::zeros(args.pre_crop_width, args.pre_crop_height, CV_8UC3);
    resize(frame, pre_crop_frame, pre_crop_frame.size(), 0, 0, INTER_LINEAR);
    Mat cropped_frame = meanShiftCrop(pre_crop_frame, 20, 0, args);
    frame = cropped_frame;

    //Draw rectangle represententing quick-and-dirty representation of tracked location
    //Rect gloveBoundingBox = fastLocateGlove(returnFrame, 60);
    //rectangle(returnFrame, gloveBoundingBox, Scalar(255,255,255));

    // Resize it to search query (resize will most likely shrink image)
    Mat normalized_frame = Mat::zeros(args.normalized_width, args.normalized_height, CV_8UC3);
    resize(frame, normalized_frame, normalized_frame.size(), 0, 0, INTER_LINEAR);
    frame = normalized_frame;

    //If in interactive mode, stretch image for display/demonstration purposes
    if (args.headless_mode == false) {
        Mat display_frame = Mat::zeros(args.display_width, args.display_height, CV_8UC3);
        resize(frame, display_frame, display_frame.size(), 0, 0, INTER_LINEAR);
        frame = display_frame;
    }

    return (frame);
}

void GloveTrack::ClassifyColors(Mat test_image, Mat test_image_sample_array, Mat& output_array) {
    if (em.isTrained() == false) {
        spdlog::get("console")->error("EM model not trained. Exiting!");
        exit(1);
    }

    int index = 0;
    for (int y = 0; y < test_image.rows; y++) {
        for (int x = 0; x < test_image.cols * 3; x = x + 3) {
            int result = em.predict(test_image_sample_array.row(index))[1];
            index++;
            //testImage[result].at<Point3i>(y, x, 0) = testImageSampleArray.at<Point3i>(y, x, 0);
            /*	    testImage.ptr<uchar>(y)[x] = classificationColor[result][0];
            testImage.ptr<uchar>(y)[x+1] = classificationColor[result][1];
            testImage.ptr<uchar>(y)[x+2] = classificationColor[result][2];*/



            /*std::cerr << "result is: " << result << " ";
              std::cerr << "classificaiton: " << resultToIndex[result] << " ";
              std::cerr << "" << classificationColor[resultToIndex[result]] << std::endl;
              std::cerr << "" << (int)testImage.ptr<uchar>(y)[x] << "," << (int)testImage.ptr<uchar>(y)[x+1] << "," << (int)testImage.ptr<uchar>(y)[x+2] << "\n";*/

            output_array.ptr<uchar>(y)[x] = classification_color[result_to_index[result]][0];
            output_array.ptr<uchar>(y)[x + 1] = classification_color[result_to_index[result]][1];
            output_array.ptr<uchar>(y)[x + 2] = classification_color[result_to_index[result]][2];
        }
    }
}

bool GloveTrack::TrainExpectationMaximizationModel(vector<Mat> raw_training_images, vector<Mat> labelled_training_images) {
    auto console = spdlog::get("console");

    int no_of_clusters = em.get<int>("nclusters");

    console->debug("Flattening set of trainingImages and labelledTraining Images into two giant sample arrays");
    //Figure out how big giant sample vector should be:
    int numSamples = 0;
    for (int i = 0; i < raw_training_images.size(); i++) {
        numSamples += (raw_training_images[i].cols * raw_training_images[i].rows);
    }
    console->debug("Sample arrays have length {}", numSamples);

    //Fill vector of samples with training images
    Mat samples = Mat::zeros(numSamples, 3, CV_32FC1);
    Mat labelledSamples = Mat::zeros(numSamples, 3, CV_32FC1);
    int samplesOffset = 0;
    for (int i = 0; i < raw_training_images.size(); i++) {
        Mat temp_samples = Mat::zeros(raw_training_images[0].rows * raw_training_images[0].cols, 3, CV_32FC1);
        Mat temp_labelled_samples = Mat::zeros(raw_training_images[0].rows * raw_training_images[0].cols, 3, CV_32FC1);
        ConvertToSampleArray(raw_training_images[i], temp_samples);
        ConvertToSampleArray(labelled_training_images[i], temp_labelled_samples);
        //Append tempSamples and tempLabelled into samples and labelledSamples respectively
        for (int j = 0; j < temp_samples.rows; j++) {
            temp_samples.row(j).copyTo(samples.row(j + samplesOffset));
            temp_labelled_samples.row(j).copyTo(labelledSamples.row(j + samplesOffset));
        }
        samplesOffset += temp_samples.rows; //increment offset
    }

    Mat initial_prob = Mat::zeros(labelledSamples.rows, no_of_clusters, CV_32FC1); //single channel matrix for trainM EM for probability  preset vals

    //fills prob matrix with initial probability
    ConvertLabelledToEMInitialTrainingProbabilityMatrix(labelledSamples, initial_prob, no_of_clusters);
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
    bool train_outcome = em.trainM(samples, initial_prob);

    //Initialize array to 0 (incase test cases don't cover all colors or labelled incorrectly etc)
    for (int i = 0; i < no_of_clusters; i++) {
        result_to_index[i] = 0;
    }
    console->debug(" Creating 'resultsToIndex' mapping between EM and classificationColor calibration");
    //Maps classificationColor array to EM result number:
    for (int i = 0; i < no_of_clusters; i++) {
        Mat test_pixel = Mat(1, 1, CV_8UC3);
        test_pixel.ptr<uchar>(0)[0] = (int) classification_color[i][0];
        test_pixel.ptr<uchar>(0)[1] = (int) classification_color[i][1];
        test_pixel.ptr<uchar>(0)[2] = (int) classification_color[i][2];
        Mat test_pixel_vector = Mat::zeros(test_pixel.cols, 3, CV_32FC1);
        ConvertToSampleArray(test_pixel, test_pixel_vector);
        SPDLOG_TRACE(console, "resultToIndex on classifcationColor #{}", i);
        int result = em.predict(test_pixel_vector.row(0))[1];
        result_to_index[result] = i;
        console->info("Result for classificationColor #{} was {}", i, result);
    }

    for (int i = 0; i < no_of_clusters; i++) {
        console->debug("resultToIndex {} is {}", i, result_to_index[i]);
    }
    console->debug("resultToIndex map constructed");
    return (train_outcome);

}

void GloveTrack::ConvertLabelledToEMInitialTrainingProbabilityMatrix(Mat pre_labelled_sample_array, Mat& prob, int num_clusters_in_em) {
    auto console = spdlog::get("console");
    console->debug("Converting to probability matrix");
    for (int i = 0; i < pre_labelled_sample_array.rows; i++) {
        bool labelled_pixel = false;
        for (int j = 0; j < num_clusters_in_em; j++) {
            //std::cerr << "here " << i << "," << j << std::endl;
            if ((pre_labelled_sample_array.ptr<float>(i)[0] == classification_color[j][0])
                    && (pre_labelled_sample_array.ptr<float>(i)[1] == classification_color[j][1])
                    && (pre_labelled_sample_array.ptr<float>(i)[2] == classification_color[j][2])) {
                prob.ptr<float>(i)[j] = 1;
                labelled_pixel = true;
            }
        }
        if (labelled_pixel == false) {//if pixel not labelled as calibrated classification color, consider pixel as background color
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

Mat GloveTrack::ConvertToSampleArray(Mat frame, Mat& output_sample_array) {
    SPDLOG_TRACE(spd::get("console"), "Started conversion");
    int index = 0;
    for (int y = 0; y < frame.rows; y++) {
        for (int x = 0; x < frame.cols * 3; x = x + 3) {
            output_sample_array.at<Vec3f>(index)[0] = (float) frame.ptr<uchar>(y)[x];
            output_sample_array.at<Vec3f>(index)[1] = (float) frame.ptr<uchar>(y)[x + 1];
            output_sample_array.at<Vec3f>(index)[2] = (float) frame.ptr<uchar>(y)[x + 2];
            index++;
        }
    }

    SPDLOG_TRACE(spdlog::get("console"), "Ended flattening");
    for (int i = 0; i < frame.rows * frame.cols; i++) {
        SPDLOG_TRACE(spdlog::get("console"), "{}", output_sample_array.at<Vec3f>(i));
    }
    return output_sample_array;
}

//Perhaps Merge cleanupImage(), locateGlove etc into this function later

Mat fastNormalizeQueryImage(Mat unprocessed_camera_frame, int threshold_brightness) {
    Rect gloveBoundingBox = fastLocateGlove(unprocessed_camera_frame, threshold_brightness);
    //rectangle(unprocessedCameraFrame, gloveBoundingBox, Scalar(0,0,0)); //Draw rectangle represententing tracked location

    Mat return_frame = unprocessed_camera_frame(gloveBoundingBox).clone(); //CROP
    return_frame = fastReduceDimensions(return_frame, 10); //shrink
    return_frame = fastClassifyColors(return_frame); //classified

    return return_frame;
}

//Need to merge cleanupImage(), fastLocateGlove etc into this function later

Mat tempNormalizeCamera(Mat unprocessed_camera_frame, int threshold_brightness) {
    Rect glove_bounding_box = fastLocateGlove(unprocessed_camera_frame, threshold_brightness);
    //rectangle(unprocessed_camera_frame, glove_bounding_box, Scalar(0,0,0)); //Draw rectangle represententing tracked location

    Mat return_frame = unprocessed_camera_frame(glove_bounding_box).clone(); //CROP
    return_frame = fastReduceDimensions(return_frame, 10); //shrink
    return_frame = classifyCamera(return_frame); //classified

    return return_frame;
}

double euclidianDist(int x1, int y1, int x2, int y2) {

    return ( sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2)));

}

std::vector<double> calcStandardDev(int population_x, int population_y, std::vector<Point> coordinates_of_valid_pixels) {
    //auto console = spdlog::get("console");
    //console->set_level(spdlog::level::debug);

    double sum_x = 0;
    double sum_y = 0;
    for (int i = 0; i < coordinates_of_valid_pixels.size(); i++) {
        sum_x += pow(coordinates_of_valid_pixels.at(i).x - population_x, 2);
        sum_y += pow(coordinates_of_valid_pixels.at(i).y - population_y, 2);
        ;
    }
    std::vector<double> to_return;
    double divisor = coordinates_of_valid_pixels.size() - 1;
    double xStdDev = sqrt(sum_x / divisor);
    to_return.push_back(xStdDev);
    double yStdDev = sqrt(sum_y / divisor);
    to_return.push_back(yStdDev);

    return (to_return);
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
Mat meanShiftCrop(Mat frame, int maximum_iterations, int minimum_distance, struct arguments args) {
    auto console = spdlog::get("console");
    //console->set_level(spdlog::level::debug);

    int cols = frame.cols;
    int rows = frame.rows;
    int channels = frame.channels();


    //Initially, we consider the  average (x,y) co-ordinates being the centre of the screen
    int curr_x = (int) cols / 2;
    int curr_y = (int) rows / 2;
    int prev_x = 0;
    int prev_y = 0;

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
    const int bandwidth_initial_height = rows;
    const int bandwidth_initial_width = cols;
    const int bandwidth_initial_x = 0;
    const int bandwidth_initial_y = 0;
    Rect bandwidth_rect = Rect(bandwidth_initial_x, bandwidth_initial_y, bandwidth_initial_width, bandwidth_initial_height);

    int current_iteration = 0;
    std::vector<double> standard_dev;

    while ((euclidianDist(curr_x, curr_y, prev_x, prev_y) > minimum_distance) && (current_iteration < maximum_iterations)) {
        current_iteration++;
        // We keep track of every valid pixel to calculate the standard deviation (used each iteration to update bandwidth update)
        std::vector<Point> coordinates_of_valid_pixels;
        //console->debug("Bandwidth rectangle is {}. Current x,y is ({},{})", bandwidthRect, currX, currY);

        double running_total_x = 0;
        double running_total_y = 0;
        int num_x = 0;
        int num_y = 0;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols * channels; j = j + channels) {//Columns are 3-channel
                Point current_pixel = Point((int) (j / channels), i);

                // Only count if non-black pixel
                if ((frame.ptr<uchar>(i)[j] != 0) && (frame.ptr<uchar>(i)[j + 1] != 0) && (frame.ptr<uchar>(i)[j + 2] != 0)) {
                    // If it's within the radius
                    if (bandwidth_rect.contains(current_pixel)) {
                        //console->debug("Adding pixel to {}",currentPixel);
                        running_total_x += (int) (j / channels);
                        running_total_y += i;
                        coordinates_of_valid_pixels.push_back(current_pixel);
                    }
                }
            }
        }

        prev_x = curr_x;
        prev_y = curr_y;
        // Logging stddev calculation running total is VERY verbose
        //SPDLOG_TRACE(spdlog::get("console"), "runningTotalX {}, runningTotalY {}", runningTotalX, runningTotalY);
        curr_x = (int) (running_total_x / coordinates_of_valid_pixels.size());
        curr_y = (int) (running_total_y / coordinates_of_valid_pixels.size());

        standard_dev = calcStandardDev(curr_x, curr_y, coordinates_of_valid_pixels);
        console->debug("Standard dev is {} {}. Mean is {}, {}", standard_dev.at(0), standard_dev.at(1), curr_x, curr_y);
        console->debug("bandwidth rect was  {}", bandwidth_rect);

        ///We update the 'bandwidth' rectangle for the next iteration of meanshift (and once converged, cropping rectangle)
        // We crop using a multiple of the standard deviation in a rectangle around the mean (exact multiplier discovered experimentally)
        float multiplier = 2;
        int x1 = curr_x - multiplier * standard_dev.at(0);
        if (x1 <= 0) {
            x1 = 0;
        }

        int y1 = curr_y - multiplier * standard_dev.at(1);
        if (y1 <= 0) {
            y1 = 0;
        }

        //OpenCV define Rect size using height and width, not absolute x2 and y2 values
        //We want right of mean by same amount, hence 2 stddev from top left corner.
        int width = multiplier * 2 * standard_dev.at(0);
        if (width >= (args.pre_crop_width - x1)) {
            width = args.pre_crop_width - x1;
        }

        int height = multiplier * 2 * standard_dev.at(1);
        if (height >= (args.pre_crop_height - x1)) {
            height = args.pre_crop_height - y1;
        }

        bandwidth_rect = Rect(x1, y1, width, height);
        console->debug("Rect is {}", bandwidth_rect);
    }

    // Finally create the cropped image with the discovered rectangle
    Mat cropped_frame = frame(bandwidth_rect);

    // We then turn the rectangle into a square (with black vertical or horizontal borders)
    Mat return_frame;
    if (cropped_frame.rows >= cropped_frame.cols) {
        return_frame = getSquareImage(cropped_frame, cropped_frame.rows);
    } else {

        return_frame = getSquareImage(cropped_frame, cropped_frame.cols);
    }

    return return_frame;
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
Rect fastLocateGlove(Mat region, int dark_threshold) {
    //LOCATE GLOVE (ie DETERMINE BOUNDING BOX):
    int num_rows = region.rows;
    int num_cols = region.cols;
    int num_channels = region.channels();

    int glove_row_start, glove_row_end, glove_col_start, glove_col_end;
    glove_col_start = num_cols; //allbackwards on purpose
    glove_row_start = num_rows;
    glove_col_end = 0;
    glove_row_end = 0;

    for (int i = 0; i < num_rows; i++) {
        for (int j = 0; j < num_cols * num_channels; j = j + num_channels) {//Columns are 3-channel
            if ((region.ptr<uchar>(i)[j] > dark_threshold)
                    && (region.ptr<uchar>(i)[j + 1] > dark_threshold)
                    && (region.ptr<uchar>(i)[j + 2] > dark_threshold)) {//if found a reasonably good looking pixel
                /*
                  if ( sqrt(pow(region.ptr<uchar>(i)[j],2) +
                  pow(region.ptr<uchar>(i)[j+1],2) +
                  pow(region.ptr<uchar>(i)[j+2],2)) > darkThreshold ){//if found a reasonably good looking pixel
                  //later, scan nearby pixels
                 */

                if (i < glove_row_start) {
                    glove_row_start = i;
                }

                if ((j / 3) < glove_col_start) {
                    glove_col_start = (j / 3);
                }
                if (i > glove_row_end) {
                    glove_row_end = i;
                }

                if ((j / 3) > glove_col_end) {
                    glove_col_end = (j / 3);
                }
            }
        }
    }

    //If something strange happend, just make tiny square
    if (glove_col_end <= glove_col_start) {
        glove_col_end = 100;
        glove_col_start = 0;
    }

    if (glove_row_end <= glove_row_start) {

        glove_row_end = 100;
        glove_row_start = 0;
    }

    return ( Rect(glove_col_start, glove_row_start, glove_col_end - glove_col_start, glove_row_end - glove_row_start));
}

//If in future may be worth it to merge this with cleanupImage so only a single cycle over fullsize image. But current is better for code clarity

Mat fastReduceDimensions(Mat region, int percent_scaling) {
    int target_height = (region.rows * percent_scaling) / 100;
    int target_width = (region.cols * percent_scaling) / 100;
    int row_skip = region.rows / target_height;
    int column_skip = region.cols / target_width;

    //Create new Mat large enough to hold glove image
    Mat shrunk_frame = Mat(target_height, target_width, CV_8UC3, Scalar(0, 0, 0));

    //Shrink image by merging adjacent pixels in square
    for (int i = 0; i < shrunk_frame.rows; ++i) {
        for (int j = 0; j < (shrunk_frame.cols * shrunk_frame.channels()); j = j + shrunk_frame.channels()) {

            shrunk_frame.ptr<uchar>(i)[j] = region.ptr<uchar>(i * row_skip)[j * column_skip];
            shrunk_frame.ptr<uchar>(i)[j + 1] = region.ptr<uchar>(i * row_skip)[j * column_skip + 1];
            shrunk_frame.ptr<uchar>(i)[j + 2] = region.ptr<uchar>(i * row_skip)[j * column_skip + 2];
            //Merge adjacent pixels:
            /*for (int k=0; k<columnSkip; k++){
              p[j] += table[region.ptr<uchar>(i*columnSkip+k)[j*rowSkip]]/(columnSkip*rowSkip);
            }
            for (int l=0; l<rowSkip; l++){
              p[j] += table[region.ptr<uchar>(i*columnSkip)[j*rowSkip+l]]/(columnSkip*rowSkip);
            }*/
        }
    }


    return (shrunk_frame.clone());
}

Mat fastClassifyColors(Mat cropped_image) {
    int num_rows = cropped_image.rows; //should be fixed later on
    int num_cols = cropped_image.cols;
    int num_channels = cropped_image.channels();

    for (int i = 0; i < num_rows; ++i) {
        for (int j = 0; j < num_cols * num_channels; j = j + num_channels) {//Columns, 3 color channels - I think OpenCV is BGR (not RGB)
            //Now we have bounding box, classify colors

            //Find smallest color difference
            int index_of_closest_color = -1;
            int smallest_delta = 444; //Biggest euclidian RGB distance is sqrt(255^2 + 255^2 + 255^2) + 1 = 442.673
            for (int k = 0; k < NUMGLOVECOLORS; k++) {
                int color_delta_of_current_pixel[3]; //BGR channels
                double euclidianDistance = 0;
                for (int l = 0; l < 3; l++) {
                    color_delta_of_current_pixel[l] = cropped_image.ptr<uchar>(i)[j + l] - classification_color[k][l];
                    euclidianDistance += pow(color_delta_of_current_pixel[l], 2);
                }
                euclidianDistance = sqrt(euclidianDistance);
                if (smallest_delta >= (int) euclidianDistance) {
                    smallest_delta = (int) euclidianDistance;
                    index_of_closest_color = k;
                }
            }
            if (index_of_closest_color != 0) { //leave blank pixel if classified as background

                cropped_image.ptr<uchar>(i)[j] = classification_color[index_of_closest_color][0];
                cropped_image.ptr<uchar>(i)[j + 1] = classification_color[index_of_closest_color][1];
                cropped_image.ptr<uchar>(i)[j + 2] = classification_color[index_of_closest_color][2];
            }
        }
    }
    return cropped_image;
}

Mat classifyCamera(Mat cropped_image) {
    int num_rows = cropped_image.rows; //should be fixed later on
    int num_cols = cropped_image.cols;
    int num_channels = cropped_image.channels();

    for (int i = 0; i < num_rows; ++i) {
        for (int j = 0; j < num_cols * num_channels; j = j + num_channels) {//Columns, 3 color channels - I think OpenCV is BGR (not RGB)
            //Now we have bounding box, classify colors

            //Find smallest color difference
            int index_of_closest_color = -1;
            int smallest_delta = 444; //Biggest euclidian RGB distance is sqrt(255^2 + 255^2 + 255^2) + 1 = 442.673
            for (int k = 0; k < NUMGLOVECOLORS; k++) {
                int color_delta_of_current_pixel[3]; //BGR channels
                double euclidian_distance = 0;
                for (int l = 0; l < 3; l++) {
                    color_delta_of_current_pixel[l] = cropped_image.ptr<uchar>(i)[j + l] - classification_color[k][l];
                    euclidian_distance += pow(color_delta_of_current_pixel[l], 2);
                }
                euclidian_distance = sqrt(euclidian_distance);
                if (smallest_delta >= (int) euclidian_distance) {
                    smallest_delta = (int) euclidian_distance;
                    index_of_closest_color = k;
                }
            }
            if (index_of_closest_color != 0) { //leave blank pixel if classified as background
                cropped_image.ptr<uchar>(i)[j] = classification_color[index_of_closest_color][0];
                cropped_image.ptr<uchar>(i)[j + 1] = classification_color[index_of_closest_color][1];
                cropped_image.ptr<uchar>(i)[j + 2] = classification_color[index_of_closest_color][2];
            }
        }
    }
    return cropped_image;
}
