#include "server.h"

void RemoteServer::connect(std::string clientType) {
    std::cout << "Remote client connected: " << clientType << std::endl;
}

void RemoteServer::fetchData() {
    std::cout << "Fetching data for remote client." << std::endl;
    Metadata metadata = extractMetadata("remote_file.txt");
    std::cout << "Author: " << metadata.author << std::endl;
    std::cout << "Title: " << metadata.title << std::endl;
    std::cout << "Description: " << metadata.description << std::endl;
    std::cout << "File Size: " << metadata.fileSize << " bytes" << std::endl;
}

void RemoteServer::updateFile(const std::string& filePath, const std::string& newData) {
    std::ofstream file(filePath, std::ios_base::app);
    if (file.is_open()) {
        file << newData << std::endl;
        file.close();
        logChange("File Updated", "Remote client updated file: " + filePath);
    } else {
        std::cerr << "Unable to open file for writing: " << filePath << std::endl;
    }
}