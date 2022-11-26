#include <unistd.h>
#include <netdb.h>
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <stdexcept>

using namespace std;

#define GN 0    // TODO: change group number

// ======================================== Declarations ===========================================

// Player ID
string PLID = "";

// Checks if the given string is composed of digits only and of length 6
bool is_valid_plid(const string &str);

// Create a socket using IPv4 and the UDP protocol
int create_udp_socket();

// Close the socket
void close_udp_socket(int sock);

// Get the IP address of the given game server
addrinfo* get_server_address(string gsIP, string gsPort);

// Function that operates the commands
void handle_command(string comm, string args, int sock, addrinfo *server);

// Request the game server to start the game
string start(int sock, addrinfo *server, string PLID);

// ============================================ Main ===============================================

int main(int argc, char *argv[]) {

    // Default Game Server's IP address and Port
    string gsIP = "";
    string gsPort = to_string(58000 + GN);

    // Input line, command and arguments
    string line, comm, args;

    // Set IP and Port ([-n gsIP] [-p gsPort])
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-n"))
            gsIP = argv[i + 1];
        if (!strcmp(argv[i], "-p"))
            gsPort = argv[i + 1];
    }

    // Get the Game Server address
    struct addrinfo *server = get_server_address(gsIP, gsPort);

    // Listen for instructions from the Player
    while (true) {
        getline(cin, line);
        stringstream(line) >> comm >> args;
        int sock = create_udp_socket();
        handle_command(comm, args, sock, server);
        close_udp_socket(sock);
    }

    // Forget the game server address
    freeaddrinfo(server);

    /*
    // Number of letters in the word, maximum number of errors allowed and number of errors made
    int numLetters, maxErrors, numErrors;
    */

    return 0;
}

// ==================================== Auxiliary Functions ========================================

int create_udp_socket() {
    int sock;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
        throw runtime_error("Error opening socket");
    return sock;
}

void close_udp_socket(int sock) {
    close(sock);
}

addrinfo* get_server_address(string gsIP, string gsPort) {
    int errcode;
    struct addrinfo hints, *server;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    errcode = getaddrinfo(gsIP.c_str(), gsPort.c_str(), &hints, &server);
    if (errcode != 0)
        throw runtime_error("Error getting address info");
    return server;
}

bool is_valid_plid(const string &str) {
    return all_of(str.begin(), str.end(), ::isdigit) && (str.length() == 6);
}

void handle_command(string comm, string args, int sock, addrinfo *server) {
    if (comm == "start" || comm == "sg") {
        if (is_valid_plid(args)) {
            PLID = args;
            start(sock, server, args);
        }
        else
            throw invalid_argument("Invalid PLID");
    }
    else if (comm == "play" || comm == "pl") {
    }
    else if (comm == "guess" || comm == "gw") {
    }
    else if (comm == "scoreboard" || comm == "sb") {
    }
    else if (comm == "hint" || comm == "h") {
    }
    else if (comm == "state" || comm == "st") {
    }
    else if (comm == "quit") {
    }
    else if (comm == "exit") {
    }
    else
        throw invalid_argument("Invalid command");
}

string start(int sock, addrinfo *server, string PLID) {
    ssize_t n;
    socklen_t addrlen;
    struct sockaddr_in addr;
    char buffer[128]; // TODO: what size should this be?

    // Send a message to the server
    string msg = "SNG " + PLID + "\n";
    n = sendto(sock, msg.c_str(), 11, 0, server->ai_addr, server->ai_addrlen);
    if (n == -1)
        throw runtime_error("Error sending start message");;

    // Receive a message from the server
    addrlen = sizeof(addr);
    n = recvfrom(sock, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
    if (n == -1)
        throw runtime_error("Error receiving start confirmation");;

    // TODO: remove this (to see if it works)
    write(1, buffer, n);
    printf(buffer);

    return buffer;
}