#include "server.h"

void AdminServer::connect(std::string clientType) {
    std::cout << "Admin client connected: " << clientType << std::endl;
}

void AdminServer::fetchData() {
    std::cout << "Fetching data for admin client." << std::endl;
    Metadata metadata = extractMetadata("admin_file.txt");
    std::cout << "Author: " << metadata.author << std::endl;
    std::cout << "Title: " << metadata.title << std::endl;
    std::cout << "Description: " << metadata.description << std::endl;
    std::cout << "File Size: " << metadata.fileSize << " bytes" << std::endl;
}

void AdminServer::updateFile(const std::string& filePath, const std::string& newData) {
    std::ofstream file(filePath, std::ios_base::app);
    if (file.is_open()) {
        file << newData << std::endl;
        file.close();
        logChange("File Updated", "Admin updated file: " + filePath);
    } else {
        std::cerr << "Unable to open file for writing: " << filePath << std::endl;
    }
}