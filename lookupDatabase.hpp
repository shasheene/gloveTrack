#ifndef LOOKUPDATABASE_H
#define LOOKUPDATABASE_H

#include "libsAndConst.hpp"

#include "math.h" //sqrt/pow
#include <iomanip>

// Abstract class for distance function

class DistanceMetric {
public:
    DistanceMetric();
    int compare(Mat image_a, Mat image_b);
private:

};

class LookupDb {
public:
    //TODO(shasheene@gmail.com): Temp, remove
    LookupDb();

    LookupDb(std::vector<Mat> search_set_of_normalized_images, std::vector<Mat> positive_examples, std::vector<Mat> negative_examples, DistanceMetric distance_function);
    void Setup();

    // TODO(shasheene@gmail.com): Make this returns handpose object
    vector<int> EstimateHandPose(Mat normalized_frame);
private:
    // Helper functions
    vector<int> ApproximateNearestNeighbor(Mat normalized_image);
    vector<int> TrueNearestNeighbor(Mat normalized_image, vector<int>);

    // Converts input image to binary string with nearest neighbor to search set encoded in hamming distance
    vector<int> ComputeBinaryCode(Mat normalized_image);
    vector<int> DoHammingDistanceComparison(vector<int> binary_string_to_lookup);


    void AddToNearestNeighbor(int euclidian_dist, int index_of_candidate,
            std::vector<int> &index_of_nearest_neighbor, std::vector<int> &dist_to_nearest_neighbor);
};

std::vector<int> queryDatabasePose(Mat isolated_frame, std::vector<Mat> comparison_images);

std::string concatStringInt(std::string part1, int part2); //Pre-C++11 standard doesn't have string class concat, atoi not standardized
#endif // LOOKUPDATABASE_H
