#ifndef GLOVETRACK_H
#define GLOVETRACK_H

#include "gloveTrackConfig.hpp"
#include "libsAndConst.hpp"

#include <iostream>

//Only use C++ library in future?
#include <stdlib.h>
#include <stdio.h>

#include <sstream>
#include <string>

#include <vector>

//Full normalization methods - (bilateral filter, expectation maximization (Guassian Mixture Model) for color classifications, meanshift (crop).
#include "isolateGlove.hpp"
#include "lookupDatabase.hpp"
#include "commandLineArguments.hpp"
#include "manifest.hpp"

//For GNU getopt command line arg parsing
// Maybe replace with c++ library
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/**
 *  Pass in a populated argument struct
 * 
 * @param args
 * @return 
 */
int runMain(struct arguments args);

#endif
