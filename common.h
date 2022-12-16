#ifndef COMMON_H
#define COMMON_H

#include <unistd.h>
#include <netdb.h>
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <cctype>
#include <vector>
#include <set>
#include <map>
#include <signal.h>
#include <stdlib.h>
#include <dirent.h>

using namespace std;

#define GN 92

// Checks if the given string is composed of
bool is_valid_port(const string &arg);      // digits only and is between 1024 and 65535
bool is_valid_plid(const string &arg);      // digits only and is of length 6
bool is_valid_letter(const string &arg);    // letters only and is of length 1
bool is_valid_word(const string &arg);      // letters only and is of length 1 or more
bool is_valid_integer(const string &arg);    // digits only and is of length 1 or more

// Get the IP address of the given game server
addrinfo* get_server_address(string gsIP, string gsPort, string prot = "udp");

// Create sockets using IPv4 and the UDP or TCP protocol
int create_socket(string prot = "udp");

// Sends a message to the server using the given socket
string request(int sock, addrinfo *server, string req);

// Reads the response from the server and writes it to a file
string read_to_file(int sock, string mode = "quiet");

#endif