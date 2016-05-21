#ifndef MANIFEST_HPP
#define	MANIFEST_HPP

#include  "include/rapidjson/document.h" 
#include  "include/rapidjson/writer.h" 
#include  "include/rapidjson/stringbuffer.h" 

#include <iostream>
#include <fstream>

#include "libsAndConst.hpp"

    /**
     * For speed of development, currently this class can hold a training set,
     * search set or evaluation sets as it just populates all the different structures
     * it finds in the json. It's the responsibility of the caller to determine
     * the json is correct and to keep track of what kind of data the manifest holds.
     * 
     * Not ideal. Will seperate at some point and can redesign with proper class
     * hierachy, but certainly good enough as of writing.
     */
class Manifest {
public:
    Manifest();
    Manifest(const Manifest& orig);
    virtual ~Manifest();
    void LoadManifest(char* path_to_load);
    void SaveManifest(char* path_to_save);

    //REMEMBER OpenCV color space is BGR, not RGB
    int num_glove_colors;
    cv::Scalar *classification_colors;
    std::vector<Mat> unnormalized_images;
    std::vector<Mat> labelled_images;
    
    /**
     *  Looks at the manifest.json file and populates all the manifest class's fields
     *  it can
     * 
     * @param path_to_load Path to manifest.json file
     */

private:

};

#endif

