#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

// Structura Metadata
struct Metadata {
    std::string author;
    std::string title;
    std::string description;
    long fileSize;
};

// Structura LogEntry pentru înregistrarea modificărilor
struct LogEntry {
    std::string timestamp;
    std::string action;
    std::string details;
};

// Funcția pentru extragerea metadatelor
Metadata extractMetadata(const std::string& filePath);

// Funcția pentru a adăuga o intrare în jurnal
void logChange(const std::string& action, const std::string& details);

// Clasa abstractă Server
class Server {
public:
    virtual void connect(std::string clientType) = 0;
    virtual void fetchData() = 0;
    virtual ~Server() {}
};

// Clasa AdminServer
class AdminServer : public Server {
public:
    void connect(std::string clientType) override;
    void fetchData() override;
    void updateFile(const std::string& filePath, const std::string& newData);
};

// Clasa RemoteServer
class RemoteServer : public Server {
public:
    void connect(std::string clientType) override;
    void fetchData() override;
    void updateFile(const std::string& filePath, const std::string& newData);
};

#endif // SERVER_H