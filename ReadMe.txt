Execute:
g++ -c metadata.cpp
g++ -c admin_server.cpp
g++ -c remote_server.cpp

g++ -o client_admin client_admin.cpp metadata.o admin_server.o
g++ -o client_remote client_remote.cpp metadata.o remote_server.o

./client_admin
./client_remote