#include "manifest.hpp"

manifest::manifest() {
}

manifest::manifest(const manifest& orig) {
}

manifest::~manifest() {
}

void manifest::load_manifest(char* path_to_load) {
    auto console = spdlog::get("console");

    console->info("Loading json file: {}", path_to_load);
    std::ifstream in(path_to_load);
    std::string contents((std::istreambuf_iterator<char>(in)),
            std::istreambuf_iterator<char>());

    // 1. Parse a JSON string into DOM.
    const char* json = contents.c_str();
    rapidjson::Document d;
    d.Parse(json);

    // 2. Modify it by DOM.
    std::string basePath = std::string(path_to_load);
    basePath = basePath.substr(0, basePath.find_last_of('/'));

    console->info("Loading gloveColors:");
    if (d["gloveColors"].IsArray() == true) {
        rapidjson::Value& gloveColors = d["gloveColors"];
        classificationColors = new cv::Scalar[gloveColors.Size()];
        console->info("There are {} colors",gloveColors.Size());
        for (int i = 0; i < gloveColors.Size(); i++) {
            classificationColors[i] = cv::Scalar(gloveColors[i][0].GetInt(), gloveColors[i][1].GetInt(), gloveColors[i][2].GetInt(), gloveColors[i][3].GetInt());
        }
    }
    console->info("gloveColors is {}",classificationColors[0]);

    rapidjson::Value& array = d["set"];
    for (rapidjson::SizeType i = 0; i < array.Size(); i++) {
        rapidjson::Value& set = array[i];
        if ((set.HasMember("unnormalized_image") == true)) {
            std::string imagePath = "";
            imagePath.append(basePath);
            imagePath.append("/");
            std::string test = set["unnormalized_image"].GetString();
            imagePath.append(test);
            console->info("{}", imagePath);
            unnormalized_images.push_back(imread(imagePath, 1));
            if (unnormalized_images.back().data == NULL) {
                console->info("Unable to read: {}", imagePath);
            }
        }
    }
    
    for (rapidjson::SizeType i = 0; i < array.Size(); i++) {
        rapidjson::Value& set = array[i];
        if ((set.HasMember("labelled_image") == true)) {
            std::string imagePath = "";
            imagePath.append(basePath);
            imagePath.append("/");
            std::string test = set["labelled_image"].GetString();
            imagePath.append(test);
            console->info("{}", imagePath);
            labelled_images.push_back(imread(imagePath, 1));
            if (labelled_images.back().data == NULL) {
                console->info("Unable to read: {}", imagePath);
            }
        }
    }
    console->info("Loaded {} unnormalized and {} labelled", unnormalized_images.size(), labelled_images.size());
}

void manifest::save_manifest(char* path_to_save) {
    rapidjson::Document d;
    // Stringify the DOM
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);

    // Output {"project":"rapidjson","stars":11}
    std::cout << buffer.GetString() << std::endl;
}