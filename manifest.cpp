#include "manifest.hpp"

Manifest::Manifest() {
}

Manifest::Manifest(const Manifest& orig) {
}

Manifest::~Manifest() {
}

void Manifest::LoadManifest(char* path_to_load) {
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
    std::string base_path = std::string(path_to_load);
    base_path = base_path.substr(0, base_path.find_last_of('/'));

    console->info("Loading gloveColors:");
    if (d["gloveColors"].IsArray() == true) {
        rapidjson::Value& gloveColors = d["gloveColors"];
        classification_colors = new cv::Scalar[gloveColors.Size()];
        console->info("There are {} colors",gloveColors.Size());
        for (int i = 0; i < gloveColors.Size(); i++) {
            classification_colors[i] = cv::Scalar(gloveColors[i][0].GetInt(), gloveColors[i][1].GetInt(), gloveColors[i][2].GetInt(), gloveColors[i][3].GetInt());
        }
    }
    console->info("gloveColors is {}",classification_colors[0]);

    rapidjson::Value& array = d["set"];
    for (rapidjson::SizeType i = 0; i < array.Size(); i++) {
        rapidjson::Value& set = array[i];
        if ((set.HasMember("unnormalized_image") == true)) {
            std::string image_path = "";
            image_path.append(base_path);
            image_path.append("/");
            std::string test = set["unnormalized_image"].GetString();
            image_path.append(test);
            console->info("{}", image_path);
            unnormalized_images.push_back(imread(image_path, 1));
            if (unnormalized_images.back().data == NULL) {
                console->info("Unable to read: {}", image_path);
            }
        }
    }
    
    for (rapidjson::SizeType i = 0; i < array.Size(); i++) {
        rapidjson::Value& set = array[i];
        if ((set.HasMember("labelled_image") == true)) {
            std::string image_path = "";
            image_path.append(base_path);
            image_path.append("/");
            std::string test = set["labelled_image"].GetString();
            image_path.append(test);
            console->info("{}", image_path);
            labelled_images.push_back(imread(image_path, 1));
            if (labelled_images.back().data == NULL) {
                console->info("Unable to read: {}", image_path);
            }
        }
    }
    console->info("Loaded {} unnormalized and {} labelled", unnormalized_images.size(), labelled_images.size());
}

void Manifest::SaveManifest(char* path_to_save) {
    rapidjson::Document d;
    // Stringify the DOM
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    d.Accept(writer);

    // Output {"project":"rapidjson","stars":11}
    std::cout << buffer.GetString() << std::endl;
}