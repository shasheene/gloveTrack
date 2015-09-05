#include "commandLineArguments.hpp"

/**
 * Parse command-line arguments to override the default values of the application-wide
 * globals such as  internal image processing width/height etc. Very useful
 * for ad-hoc experimenting trading-off between accuracy and speed.
 * 
 */
void parseCommandLineArgs(int argc, char** argv, struct arguments &args) {
  char *cvalue = NULL;
  int index;
  int c;

  int this_option_optind = optind ? optind : 1;
  int option_index = 0;
  static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"verbose", optional_argument, 0, 'v'},
    {"version", no_argument, 0, 0},
    {"testing-set-location", required_argument, 0, 0},
    {"training-set-location", required_argument, 0, 0},
    {"pose-set-location", required_argument, 0, 0},
    
    {"normalized-width", required_argument, 0, 0},
    {"normalized-height", required_argument, 0, 0},
    {"processing-width", required_argument, 0, 0},
    {"processing-height", required_argument, 0, 0},
    {"display-width", required_argument, 0, 0},
    {"display-height", required_argument, 0, 0},
    
    {"interactive-mode", no_argument, 0, 'i'},
    {"capture-device", required_argument, 0, 'c'},
    {0, 0, 0, 0}
  };



  
  opterr = 0;
  while ((c = getopt_long(argc, argv, "ic:v:hn",long_options, &option_index)) != -1)
    switch (c) {
      case 0:
        char* long_option;
        long_option= (char* )long_options[option_index].name;
        std::cout << "option " << long_option;
        if (optarg) {
          std::cout << " with arg " << optarg;
        }
        std::cout << std::endl;
        
         if (0 == strcmp("testing-set-location",long_option)) {
          //
        } else if (0 == strcmp("training-set-height",long_option)) {
          //
        } else if (0 == strcmp("pose-set-height",long_option)) {
          //
        } else if (0 == strcmp("normalized-width",long_option)) {
          args.normalizedWidth = atoi(optarg);
        } else if (0 == strcmp("normalized-height",long_option)) {
          args.normalizedHeight = atoi(optarg);
        } else if (0 == strcmp("processing-width",long_option)) {
          args.processingWidth = atoi(optarg);
        } else if (0 == strcmp("processing-height",long_option)) {
          args.processingHeight = atoi(optarg);
        } else if (0 == strcmp("display-width",long_option)) {
          args.displayWidth = atoi(optarg);
        } else if (0 == strcmp("display-height",long_option)) {
          args.displayHeight = atoi(optarg);
        }
        break;

      case '0':
      case '1':
      case 'h':
        std::cout << "gloveTrack" << " Version " << Glovetrack_VERSION_MAJOR << " " << Glovetrack_VERSION_MINOR << std::endl;
        std::cout << "Usage: ./gloveTrack [OPTION]" << std::endl;
        std::cout << "Example: ./gloveTrack -i" << std::endl;
        std::cout << "   -i,--interactive-mode              Shows processing on-screen in real-time" << std::endl;
        std::cout << "   -c,--capture-device=INDEX          Select video capture device (eg, built-in webcam might be 0)" << std::endl;
        std::cout << "   -v,--verbose                       Verbose debug output (to stdout)" << std::endl;

        std::cout << "      --training-set=PATH             Directory containing labelled and unlabelled images for 'supervised' training of the statistical model" << std::endl;
        std::cout << "      --testing-set=PATH|VIDEO        Directory (or video file) containing raw images for offline (non live-camera) processing, for demonstration and development (Cannot be used with -c)" << std::endl;
        std::cout << "      --normalized-width=SIZE         Width of output image after processing" << std::endl;
        std::cout << "      --normalized-height=SIZE        Height of output image after processing" << std::endl;

        std::cout << "   -n,--number-of-glove-colors=NUM    Number of unique colors on the glove" << std::endl;

        std::cout << "      --processing-width=SIZE         Resize to width before further processing (input raw camera frames potentially larger than 3840x2160 resolution)" << std::endl;
        std::cout << "      --processing-height=SIZE        Resize to height before further processing " << std::endl;

        std::cout << "      --display-width=SIZE            Resize to width before display to user (interactive mode only)" << std::endl;
        std::cout << "      --display-height=SIZE           Resize to height before display to user " << std::endl;


        std::cout << "      --pose-set=PATH                 Base directory containing pre-computed searching set" << std::endl;
        std::cout << "      --version                       Print version information" << std::endl;
        
        exit(0);
        break;
      case 'v':
        verbosity = atoi(optarg);
        break;
      case 'i':
        std::cout << "Interactive Mode (non-headless)" << std::endl;
        realTimeMode = false;
        break;
      case 's':
        std::cout << "Slow mode enabled (even more info than debug mdoe" << std::endl;
        slowMode = true;
        break;
      case 'c':
        videoCaptureDeviceNumber = atoi(optarg);
        std::cout << "Video capture device selected is " << videoCaptureDeviceNumber  << std::endl;
        break;
      case 'n':
        atoi(optarg);
        std::cout << "Video capture device selected is ???" << std::endl;
        break;
      case '?':
        break;
      default:
        printf("?? getopt returned character code 0%o ??\n", c);
        abort();
    }
  printf("Video capture device number is %s\n", cvalue);

  for (index = optind; index < argc; index++)
    printf("Non-option argument %s\n", argv[index]);
}
