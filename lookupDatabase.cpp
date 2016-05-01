#include "lookupDatabase.hpp"

LookupDb::LookupDb() {

}

LookupDb::LookupDb(std::vector<Mat> search_set_of_normalized_images, std::vector<Mat> positive_examples, std::vector<Mat> negative_examples, DistanceMetric distance_function) {
}

void LookupDb::Setup() {
}

vector<int> LookupDb::EstimateHandPose(Mat normalized_frame) {
    vector<int> test;
    return test;
}

/*        vector<int> ApproximateNearestNeighbor(Mat normalizedImage);
       vector<int> TrueNearestNeighbor(Mat normalizedImage, vector<int>);
        
       // Converts input image to binary string with nearest neighbor to search set encoded in hamming distance
       vector<int> ComputeBinaryCode(Mat normalizedImage);
       vector<int> DoHammingDistanceComparison(vector<int> binaryStringToLookup);
 */

//TEMPORARY QUCIK AND DIRTY INSERTION SORT. O(n^2) is ok as function used in offline (ie non-realtime) verification

void addToNearestNeighbor(int euclidian_dist, int index_of_candidate,
        std::vector<int> &index_of_nearest_neighbor, std::vector<int> &dist_to_nearest_neighbor) {
    std::vector<int>::iterator it = dist_to_nearest_neighbor.begin();
    bool inserted = false;
    for (int i = 0; i < dist_to_nearest_neighbor.size(); i++) {
        if (euclidian_dist <= dist_to_nearest_neighbor.at(i) && inserted == false) {
            it = index_of_nearest_neighbor.begin();
            index_of_nearest_neighbor.insert(it + i, index_of_candidate);

            it = dist_to_nearest_neighbor.begin();
            dist_to_nearest_neighbor.insert(it + i, euclidian_dist);
            inserted = true;
        }
    }
    if (dist_to_nearest_neighbor.size() == 0 || inserted == false) {
        dist_to_nearest_neighbor.push_back(euclidian_dist);
        index_of_nearest_neighbor.push_back(index_of_candidate);
    }
}

std::vector<int> queryDatabasePose(Mat curr, std::vector<Mat> comparison_images) {
    //Using the Vec3b slowed current from 15ms to 30ms, so usig pointers

    std::vector<int> dist_to_nearest_neighbor;
    std::vector<int> index_of_nearest_neighbor;
    int dark_threshold = 180;
    for (int q = 0; q < comparison_images.size(); q++) {
        std::cerr << "Comparing to comparison image: " << q;

        int running_total_hamming_dist = 0;
        for (int i = 0; i < curr.rows; ++i) {
            uchar *current_pixel = curr.ptr<uchar>(i);
            for (int j = 0; j < curr.cols * curr.channels(); j = j + curr.channels()) {
                int color_delta = 0;
                //calculateDistanceMetric. Over every pixel of comparison image, weighted by distnace
                for (int k = 0; k < comparison_images.at(q).rows; ++k) {
                    uchar *db_pixel = comparison_images.at(q).ptr<uchar>(k);
                    for (int l = 0; l < comparison_images.at(q).cols * comparison_images.at(q).channels(); l = l + 3) {
                        float euclidian_pixel_distance = sqrt(pow((i - k), 2) + pow((j / 3 - l / 3), 2));
                        if (euclidian_pixel_distance <= 7) { //we only consider pixels within 7 pixels radius. For testing 2 pixels is good
                            //std::cerr << " Comparing to pixel (" << k << "," << l/3 << ") which has euclidian 2D distance " << euclidianPixelDistance << " " << "\n";      
                            color_delta = sqrt(pow((current_pixel[j] - db_pixel[l]), 2)
                                    + pow((current_pixel[j + 1] - db_pixel[l + 1]), 2)
                                    + pow((current_pixel[j + 2] - db_pixel[l + 2]), 2));

                            if (euclidian_pixel_distance == 0) {
                                euclidian_pixel_distance = 1; //if same pixel, don't divide by zero but give big weight
                            }
                            color_delta = color_delta / euclidian_pixel_distance; //and weigh the pixels lower if further
                        }
                    }
                }

                if ((current_pixel[j] + current_pixel[j + 1] + current_pixel[j + 2]) > dark_threshold) {
                    running_total_hamming_dist += (int) color_delta;
                }
            }
            //std::cerr << "running total of this row #" << i << ": " << running_total_hamming_dist << " on image " << q << "\n";
        }

        addToNearestNeighbor(running_total_hamming_dist, q, index_of_nearest_neighbor, dist_to_nearest_neighbor);
        spdlog::get("console")->info("working: {} distance was {}", running_total_hamming_dist, q);
    }
    return index_of_nearest_neighbor;
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

std::string concatStringInt(std::string part1, int part2) {
    std::stringstream ss;
    ss << part1 << std::setw(4) << std::setfill('0') << part2; //append leading zeroes so format of read in is same as Blender output png
    return (ss.str());
}




