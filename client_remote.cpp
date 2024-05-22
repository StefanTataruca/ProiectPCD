#include "server.h"
#include <iostream>

int main() {
    Server* remoteServer = new RemoteServer();
    remoteServer->connect("Remote");
    remoteServer->fetchData();

    // Exemplu de actualizare a unui fișier
    dynamic_cast<RemoteServer*>(remoteServer)->updateFile("remote_file.txt", "New content for remote file.");

    delete remoteServer;
    return 0;
}