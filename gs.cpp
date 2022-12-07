#include <netdb.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

#define GN 92

// ======================================== Declarations ===========================================

int create_socket(string prot = "udp");
pair<string, string> get_word(string word_file);

string gsPort = to_string(58000 + GN);
bool verbose = false;

// ============================================ Main ===============================================

int main(int argc, char* argv[]) {
    struct addrinfo hints, *res;
    socklen_t addrlen;
    struct sockaddr_in address;
    char buffer[1024]; //copilot set this to 1024 hello copilot

    if (argc < 1)
        throw invalid_argument("No word file specified");
    else if (argc == 2) {
        string word_file = argv[1];
        string word, cat;
        // TODO: is there a better way to do this?
        pair<string, string> word_cat = get_word(word_file);
        word = word_cat.first;
        cat = word_cat.second;
        cout << word << endl << cat << endl;
    }
    else if (argc == 4) {
        if (strcmp(argv[3], "-p"))
            gsPort = argv[3];
    }
    else if (argc > 4) {
        if (strcmp(argv[4], "-v"))
            verbose = true;
    }
    else
        throw invalid_argument("Invalid number of arguments");

    // Open sockets (#TODO put this in a function)
    int udp_sock = create_socket();
    int tcp_sock = create_socket("tcp");

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    // Get address info
    if (getaddrinfo(NULL, gsPort.c_str(), &hints, &res) != 0)
        throw runtime_error("getaddrinfo() failed");

    // Bind socket
    if (bind(udp_sock, res->ai_addr, res->ai_addrlen) < 0)
        throw runtime_error("Error binding UDP socket");
    if (bind(tcp_sock, res->ai_addr, res->ai_addrlen) < 0)
        throw runtime_error("Error binding TCP socket");
    if (listen(tcp_sock, 5) < 0)
        throw runtime_error("Error listening on TCP socket");

    addrlen = sizeof(address);

    //-----------------

    fd_set readfds;
    int nready;
    int newfd;
    pid_t pid;
    ssize_t n;

    // Listen to sockets
    while (true) {
        FD_SET(udp_sock, &readfds);
        FD_SET(tcp_sock, &readfds);

        int max_sock = max(udp_sock, tcp_sock);
        // select() blocks until there is data to read, sets the bytemap (readfs) to 1 for the socket which has data to read
        nready = select(max_sock + 1, &readfds, NULL, NULL, NULL);

        // Logic for UDP requests
        if (FD_ISSET(udp_sock, &readfds)) {
            n = recvfrom(udp_sock, buffer, 1024, 0, (struct sockaddr *) &address, &addrlen);
            if (n < 0)
                throw runtime_error("Error receiving UDP message");
            // handle_udp(buffer, n); // Handle function should fork if the user is new
            if (verbose) continue;
                // TODO implement verbose
        }
        // Logic for TCP requests
        else if (FD_ISSET(tcp_sock, &readfds)) {
            if((newfd = accept(tcp_sock, (struct sockaddr *) &address, &addrlen)) < 0)
                throw runtime_error("Error accepting TCP connection");

            n = read(newfd, buffer, 1024);
            if (n < 0)
                throw runtime_error("Error reading TCP message");
            // handle_tcp(buffer, n); // TODO implement handle_tcp
            if (verbose) continue;
                // TODO implement verbose
            }
        }
        return 0;
    }

// ==================================== Auxiliary Functions ========================================

// ======================== Files ============================
pair<string, string> get_word(string word_file) {
    string line, word, cat;
    vector<string> lines;
    int numLines = 0;
    srand(time(NULL));

    // Open file containing words
    ifstream input(word_file);
    while(getline(input, line)) {
        ++numLines;
        lines.push_back(line);
    }
    int random = rand() % numLines;
    stringstream(lines[random]) >> word >> cat;
    return {word, cat};
}

// ======================== Game Logic ============================

// ======================== Network ============================

int create_socket(string prot) {
    int type = (prot == "tcp") ? SOCK_STREAM : SOCK_DGRAM;
    int sock = socket(AF_INET, type, 0);
    if (sock == -1)
        throw runtime_error("Error opening " + prot + " socket.");
    return sock;
}