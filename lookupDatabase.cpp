#include "lookupDatabase.hpp"
#include <map>

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


//temp helper. replace with lookup table
int matching(unsigned char pixel[3],  Scalar glove_colors[NUMGLOVECOLORS]) {
    for (int i=0; i<NUMGLOVECOLORS;i++) {
        unsigned char glove[3] = {(uchar)glove_colors[i][0], (uchar)glove_colors[i][1], (uchar)glove_colors[i][2]};
        if (glove[0]==pixel[0] && glove[1]==pixel[1] && glove[2]==pixel[2]) {
            return i;
        }
    }
    std::cout << "Pixel not in classification_colors array. Perhaps resized with wrong interpolation" << std::endl;
    exit(0);
}

void convertNormalizedMatToIndexArray(Mat curr, Scalar glove_colors[NUMGLOVECOLORS], unsigned char output[50][50]) {
    for (int i = 0; i < curr.rows; i++) {
        for (int j = 0; j < curr.cols * 3; j=j+3) {
            unsigned char lookup[3] = {curr.ptr<uchar>(i)[j], curr.ptr<uchar>(i)[j+1], curr.ptr<uchar>(i)[j+2]};
            output[i][j] = (unsigned char) matching(lookup, glove_colors);
        }
    }
}

bool orderBiggestToSmallest (struct dbElement* i, struct dbElement* j) {
    return (i->distance_metric > j->distance_metric);
}
    
/**
 * Caller responsibility to free all pose struct pointers in return vector.
 */
std::vector<struct dbElement*> queryDatabasePose(Mat curr, std::vector<Mat> comparison_images) {
    //Using the Vec3b slowed current from 15ms to 30ms, so usig pointers

    std::vector<struct dbElement*> index_of_nearest_neighbor;
    
    for (int q = 0; q < comparison_images.size(); q++) {
        //std::cerr << "Comparing to comparison image: " << q;
        
        struct dbElement* db_element = (struct dbElement*) malloc(sizeof(dbElement));
        db_element->index = q;
        db_element->distance_metric = 0;
        for (int i = 0; i < curr.rows; ++i) {
            uchar *current_pixel = curr.ptr<uchar>(i);
            for (int j = 0; j < curr.cols * curr.channels(); j = j + curr.channels()) {
                int color_delta = 0;
                
                uchar *db_pixel = comparison_images.at(q).ptr<uchar>(i);
                if (current_pixel[j]==db_pixel[j] && current_pixel[j + 1]==db_pixel[j+1] && current_pixel[j + 2]==db_pixel[j+2]) {
                    db_element->distance_metric += 1;
                }
            }
            //std::cerr << "running total of this row #" << i << ": " << running_total_hamming_dist << " on image " << q << "\n";
        }
        index_of_nearest_neighbor.push_back(db_element);
        //spdlog::get("console")->info("Candidate image {} had distance {}", q, running_total_hamming_dist);
    }
    std::sort(index_of_nearest_neighbor.begin(), index_of_nearest_neighbor.end(), orderBiggestToSmallest);
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




