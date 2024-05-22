#include "server.h"
#include <ctime>

Metadata extractMetadata(const std::string& filePath) {
    Metadata metadata;
    std::ifstream file(filePath);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("Author:") == 0) {
                metadata.author = line.substr(8);
            } else if (line.find("Title:") == 0) {
                metadata.title = line.substr(7); 
            } else if (line.find("Description:") == 0) {
                metadata.description = line.substr(13);
            }
        }
        file.clear(); 
        file.seekg(0, std::ios::end);
        std::streampos fileSize = file.tellg();
        if (fileSize != -1) {
            metadata.fileSize = fileSize;
        } else {
            metadata.fileSize = -1; 
        }
        file.close();
    } else {
        metadata.fileSize = -1;
    }
    return metadata;
}
std::string getCurrentTime() {
    std::time_t now = std::time(nullptr);
    char buf[80];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return std::string(buf);
}

void logChange(const std::string& action, const std::string& details) {
    std::ofstream logFile("file_changes.log", std::ios_base::app);
    if (logFile.is_open()) {
        logFile << getCurrentTime() << " - " << action << ": " << details << std::endl;
        logFile.close();
    } else {
        std::cerr << "Unable to open log file." << std::endl;
    }
}