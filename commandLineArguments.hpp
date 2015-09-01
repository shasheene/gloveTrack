#ifndef COMMANDLINEARGUMENTS_H
#define COMMANDLINEARGUMENTS_H

#include "libsAndConst.hpp"


//For GNU getopt command line arg parsing
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>


void parseCommandLineArgs(int argc, char** argv);

#endif
