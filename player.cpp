#include <unistd.h>
#include <netdb.h>
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <algorithm>

using namespace std;

#define GN 0    // TODO: change group number

// ======================================== Declarations ===========================================

// Checks if the given string is composed of digits only and of length 6
bool is_valid_plid(const string &str);

// Create a socket using IPv4 and the UDP protocol
int create_udp_socket();

// Close the socket
void close_udp_socket(int sock);

// Get the IP address of the given game server
addrinfo* get_server_address(string gsIP, string gsPort);

// Function that operates the commands
string handle_command(string comm, string args, int sock, addrinfo *server);

string start(int sock, addrinfo *server, string PLID);

// ============================================ Main ===============================================

int main(int argc, char *argv[]) {

    // Default Game Server's IP address and Port
    string gsIP = "";
    string gsPort = to_string(58000 + GN);

    // Player ID and Input line, comm and arguments
    string PLID;
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
        PLID = handle_command(comm, args, sock, server);
        close_udp_socket(sock);
    }

    // Forget the game server address
    freeaddrinfo(server);

    /*
    // Number of letters in the word, Maximum number of errors allowed and number of errors made
    int numLetters;
    int maxErrors;
    int numErrors;

    // Talks to server and sets game to True if ID is valid
    game = true;
    */

    return 0;
}

// ==================================== Auxiliary Functions ========================================

int create_udp_socket() {
    int sock;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
        exit(1);
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
    if (errcode != 0) exit(1);
    return server;
}

bool is_valid_plid(const string &str) {
    return all_of(str.begin(), str.end(), ::isdigit) && (str.length() == 6);
}

string handle_command(string comm, string args, int sock, addrinfo *server) {
    if (comm == "start" || comm == "sg") {
        if (!is_valid_plid(args))
            cout << "Invalid PLID" << endl;
        else {
            start(sock, server, args);
            return args;
        }
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
        cout << "Invalid command" << endl;
    return ""; // TODO: what to do in this case?
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
        exit(1);

    // Receive a message from the server
    addrlen = sizeof(addr);
    n = recvfrom(sock, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
    if(n == -1)
        exit(1);

    // TODO: remove this
    write(1, buffer, n);
    printf(buffer);

    return buffer;
}