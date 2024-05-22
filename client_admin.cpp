#include "server.h"
#include <iostream>

int main() {
    Server* adminServer = new AdminServer();
    adminServer->connect("Admin");
    adminServer->fetchData();

    // Exemplu de actualizare a unui fișier
    dynamic_cast<AdminServer*>(adminServer)->updateFile("admin_file.txt", "New content for admin file.");

    delete adminServer;
    return 0;
}