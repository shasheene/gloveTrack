#ifndef COMMANDLINEARGUMENTS_H
#define COMMANDLINEARGUMENTS_H

#include "libsAndConst.hpp"


//For GNU getopt command line arg parsing
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

/**
 * Struct containing parsed command-line parameters
 * 
 * They are populated at startup with reasonable defaults but are overwritten by 
 * the relevant command-line parameters.
 * 
 */
struct arguments {
    bool headless_mode;
    bool generate_search_set_mode;
    bool display_input_images;
    bool lookup_db;
        
    int video_capture_device;
    int num_glove_colors;

    int pre_crop_width;
    int pre_crop_height;

    
    int processing_width;
    int processing_height;

    int normalized_width;
    int normalized_height;

    int display_width;
    int display_height;
    
    char* training_set_manifest;
    char* evaluation_set_manifest;
    char* search_set_manifest;
    char* input_video_file;
    
    bool save_normalized_images;
};

void parseCommandLineArgs(int argc, char** argv, struct arguments &args);

#endif
