#include "commandLineArguments.hpp"

/**
 * Parse command-line arguments to override the default values of the application-wide
 * globals such as  internal image processing width/height etc. Very useful
 * for ad-hoc experimenting trading-off between accuracy and for speed.
 * 
 */
void parseCommandLineArgs(int argc, char** argv) {
  char *cvalue = NULL;
  int index;
  int c;

  opterr = 0;
  while ((c = getopt(argc, argv, "vhdnsc:")) != -1)
    switch (c) {
      case 'v':
        std::cout << "gloveTrack" << " Version " << Glovetrack_VERSION_MAJOR
                << " " << Glovetrack_VERSION_MINOR << std::endl;
        exit(0);
        break;
      case 'h':
        std::cout << "Usage: ./gloveTrack [-d|-v]" << std::endl;
        exit(0);
        break;
      case 'd':
        std::cout << "Debug mode enabled" << std::endl;
        verbosity = 1; //make this take debug level later
        break;
      case 'n':
        std::cout << "Nearest neighbor test mode(Realtime disabled)" << std::endl;
        realTimeMode = false;
        break;
      case 's':
        std::cout << "Slow mode enabled (even more info than debug mdoe" << std::endl;
        slowMode = true;
        break;
      case 'c':
        videoCaptureDeviceNumber = atoi(optarg);
        fprintf(stderr, "Video capture device selected is %d\n", videoCaptureDeviceNumber);
        break;
      case '?':
        if (optopt == 'c')
          fprintf(stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint(optopt))
          fprintf(stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf(stderr,
                "Unknown option character `\\x%x'.\n",
                optopt);
        exit(1);
      default:
        abort();
    }
  printf("Video capture device number is %s\n", cvalue);

  for (index = optind; index < argc; index++)
    printf("Non-option argument %s\n", argv[index]);
}
