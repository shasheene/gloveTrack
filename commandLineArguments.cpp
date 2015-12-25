#include "commandLineArguments.hpp"

/**
 * Parse command-line arguments to override the default values of the application-wide
 * globals such as  internal image processing width/height etc. Very useful
 * for ad-hoc experimenting trading-off between accuracy and speed.
 * 
 */
void parseCommandLineArgs(int argc, char** argv, struct arguments &args) {
    auto console = spdlog::get("console");

    char *cvalue = NULL;
    int index;
    int c;

    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 0},
        {"headless-mode", no_argument, 0, 0},
        {"display-input-images", no_argument, 0, 'd'},
        {"save-normalized-images", no_argument, 0, 's'},
        {"training-set-manifest", required_argument, 0, 0},
        {"evaluation-set-manifest", required_argument, 0, 0},
        {"pose-set-manifest", required_argument, 0, 0},

        {"precrop-width", required_argument, 0, 0},
        {"precrop-height", required_argument, 0, 0},

        {"processing-width", required_argument, 0, 0},
        {"processing-height", required_argument, 0, 0},

        {"normalized-width", required_argument, 0, 0},
        {"normalized-height", required_argument, 0, 0},

        {"display-width", required_argument, 0, 0},
        {"display-height", required_argument, 0, 0},

        {"input-video", required_argument, 0, 'i'},
        {"capture-device", required_argument, 0, 'c'},
        {0, 0, 0, 0}
    };




    opterr = 0;
    while ((c = getopt_long(argc, argv, "ic:v:hn", long_options, &option_index)) != -1)
        switch (c) {
            case 0:
                char* long_option;
                long_option = (char*) long_options[option_index].name;
                if (optarg) {
                    console->info("Option {} given with argument {}", long_option, optarg);
                } else {
                    console->info("Option {} given");
                }

                if (0 == strcmp("headless-mode", long_option)) {
                    args.headlessMode = true;
                } else if (0 == strcmp("display-input-images", long_option)) {
                    args.displayInputImages = true;
                } else if (0 == strcmp("training-set-manifest", long_option)) {
                    args.trainingSetManifest = strdup(optarg);
                } else if (0 == strcmp("evaluation-set-manifest", long_option)) {
                    args.evaluationSetManifest = strdup(optarg);
                } else if (0 == strcmp("pose-set-manifest", long_option)) {
                    args.searchSetManifest = strdup(optarg);
                } else if (0 == strcmp("input-video", long_option)) {
                    args.inputVideoFile = strdup(optarg);
                } else if (0 == strcmp("processing-width", long_option)) {
                    args.processingWidth = atoi(optarg);
                } else if (0 == strcmp("processing-height", long_option)) {
                    args.processingHeight = atoi(optarg);
                } else if (0 == strcmp("precrop-width", long_option)) {
                    args.preCropWidth = atoi(optarg);
                } else if (0 == strcmp("precrop-height", long_option)) {
                    args.preCropHeight = atoi(optarg);
                } else if (0 == strcmp("normalized-width", long_option)) {
                    args.normalizedWidth = atoi(optarg);
                } else if (0 == strcmp("normalized-height", long_option)) {
                    args.normalizedHeight = atoi(optarg);
                } else if (0 == strcmp("display-width", long_option)) {
                    args.displayWidth = atoi(optarg);
                } else if (0 == strcmp("display-height", long_option)) {
                    args.displayHeight = atoi(optarg);
                } else if (0 == strcmp("capture-device", long_option)) {
                    args.videoCaptureDevice = atoi(optarg);
                } else if (0 == strcmp("save-normalized-images", long_option)) {
                    args.saveNormalizedImages = true;
                }
                break;

            case '0':
            case '1':
            case 'h':
                std::cout << "gloveTrack" << " Version " << Glovetrack_VERSION_MAJOR << " " << Glovetrack_VERSION_MINOR << std::endl;
                std::cout << "Usage: ./gloveTrack [OPTION]" << std::endl;
                std::cout << "Example: ./gloveTrack -i evaluationSet/vid.mp4" << std::endl;
                std::cout << std::endl;
                std::cout << "   -i,--input-video=FILE              Use pre-recorded video file as input image stream" << std::endl;
                std::cout << "   -c,--capture-device=INDEX          Select video capture device (eg, built-in webcam might be 0)" << std::endl;
                std::cout << "   -h,--headless-mode                 Don't display processing on-screen" << std::endl;
                std::cout << "   -d,--display-input-images          Displays raw input frames on screen as processing takes place" << std::endl;
                
                std::cout << "      --training-set-manifest=FILE    Manifest file detailing the labelled and unlabelled images for 'supervised' training of the statistical model" << std::endl;
                std::cout << "      --evaluation-set-manifest=FILE  Manifest file detailing the raw images for offline (non live-camera) processing, for demonstration and development (Cannot be used with -c)" << std::endl;
                std::cout << "      --pose-set-manifest=PATH        Manifest file detailing the pre-computed searching set" << std::endl;

                std::cout << "      --processing-width=SIZE         Resize to width after cropping for any further processing (for development and testing)" << std::endl;
                std::cout << "      --processing-height=SIZE        Resize to height after cropping for any further processing" << std::endl;

                std::cout << "      --precrop-width=SIZE            Resize to width before color classification and cropping (input raw camera frames potentially larger than 3840x2160 resolution)" << std::endl;
                std::cout << "      --precrop-height=SIZE           Resize to height before color classification and cropping " << std::endl;

                std::cout << "      --normalized-width=SIZE         Width of output image after processing (the normalized search query size)" << std::endl;
                std::cout << "      --normalized-height=SIZE        Height of output image after processing" << std::endl;

                std::cout << "      --display-width=SIZE            Resize to width before display to user (interactive mode only)" << std::endl;
                std::cout << "      --display-height=SIZE           Resize to height before display to user " << std::endl;
                std::cout << std::endl;

                std::cout << "   -s,--save-normalized-images        Save normalized image (slow)" << std::endl;

                std::cout << "      --version                       Print version information" << std::endl;

                exit(0);
                break;
            case 'd':
                args.displayInputImages = true;
                break;
            case 'i':
                //fix this
                args.inputVideoFile = strdup(optarg);
                break;
            case 's':
                args.saveNormalizedImages = true;
                break;
            case '?':
                break;
            default:
                console->error("getopt returned character code {}", c);
                abort();
        }

    for (index = optind; index < argc; index++)
        console->error("Non-option argument %s", argv[index]);
}
