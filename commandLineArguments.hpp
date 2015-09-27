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
    bool headlessMode;
    int videoCaptureDevice;
    int numGloveColors;

    int processingWidth;
    int processingHeight;

    int normalizedWidth;
    int normalizedHeight;

    int displayWidth;
    int displayHeight;
    
    char* inputVideo;
    char* trainingSetManifest;
    char* evaluationSetManifest;
    char* poseSetManifest;
};

void parseCommandLineArgs(int argc, char** argv, struct arguments &args);

#endif
