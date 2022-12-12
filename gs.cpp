#include <netdb.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <map>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

#define GN 92

// ======================================== Declarations ===========================================

void handle_request(char* buffer, int n, bool verbose);

int create_socket(string prot = "udp");

pair<string, string> get_word(string word_file);

map<int, int> client_process_map;

string gsIP = "localhost";
string gsPort = to_string(58000 + GN);
bool verbose = false;

int udp_socket;
int tcp_socket;

// ============================================ Main ===============================================

int main(int argc, char* argv[]) {
    struct addrinfo hints, *res;
    socklen_t addrlen;
    struct sockaddr_in address;
    char buffer[1024]; //copilot set this to 1024

    cout << "argc: " << argc << endl;

    if (argc < 1)
        throw invalid_argument("No word file specified");
    else if (argc >= 1) {
        string word_file = argv[1];
        string word, cat;
        pair<string, string> word_cat = get_word(word_file);
        word = word_cat.first;
        cat = word_cat.second;
        // cout << "1 " << word << " " << cat << endl;
        // cout << "2 " << word_file << endl;
    }
    for (int i = 2; i < argc; ++i) {
        if (!strcmp(argv[i], "-p"))
            gsPort = argv[i + 1];
        else if (!strcmp(argv[i], "-v"))
            verbose = true;
    }

    // Open sockets (#TODO put this in a function)
    udp_socket = create_socket();
    tcp_socket = create_socket("tcp");

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    // Get address info
    if (getaddrinfo(NULL, gsPort.c_str(), &hints, &res) != 0)
        throw runtime_error("getaddrinfo() failed");

    // Bind socket
    if (bind(udp_socket, res->ai_addr, res->ai_addrlen) < 0)
        throw runtime_error("Error binding UDP socket");
    if (bind(tcp_socket, res->ai_addr, res->ai_addrlen) < 0)
        throw runtime_error("Error binding TCP socket");
    if (listen(tcp_socket, 5) < 0)
        throw runtime_error("Error listening on TCP socket");

    addrlen = sizeof(address);

    //-----------------

    fd_set readfds;
    int nready;
    int newfd;
    //pid_t pid;
    ssize_t n;
    int i = 0;

    // Listen to sockets
    while (true) {
        cout << "hello" << i++ << endl;
        FD_SET(udp_socket, &readfds);
        FD_SET(tcp_socket, &readfds);

        int max_sock = max(udp_socket, tcp_socket);
        /* select() blocks until there is data to read, sets the bytemap (readfs) to 1 for the socket which has data to read */
        nready = select(max_sock + 1, &readfds, NULL, NULL, NULL);

        // Logic for UDP requests
        if (FD_ISSET(udp_socket, &readfds)) {
            n = recvfrom(udp_socket, buffer, 1024, 0, (struct sockaddr *) &address, &addrlen);
            if (n < 0)
                throw runtime_error("Error receiving UDP message");
            cout << "Received UDP message: " << buffer << endl;
            handle_request(buffer, n, verbose); // Handle function should fork if the user is new
        }
        // Logic for TCP requests
        else if (FD_ISSET(tcp_socket, &readfds)) {
            if((newfd = accept(tcp_socket, (struct sockaddr *) &address, &addrlen)) < 0)
                throw runtime_error("Error accepting TCP connection");

            n = read(newfd, buffer, 1024);
            if (n < 0)
                throw runtime_error("Error reading TCP message");
            cout << "Received TCP message: " << buffer << endl;
            handle_request(buffer, n, verbose); // TODO implement handle_tcp
        }
    }
    return 0;
}

void handle_request(char* buffer, int n, bool verbose) {
    /*commands can be SNG, PLG, PWG, QUT, REV, GSB, GHL, STA*/
    string comm;
    string plid_string;

    stringstream ss(buffer);
    ss >> comm;
    ss >> plid_string;

    int plid = stoi(plid_string);

    // Check if the player's ID already has a process associated with it
    auto it = client_process_map.find(plid);
    if (it = client_process_map.end()) {
        // If not, fork a new process
        pid_t pid = fork();
        if (pid < 0)
            throw runtime_error("Error forking process");
        else if (pid == 0) {
            // Child process
    }


    if (comm = "SNG") {

    } else if (comm = "PLG") {

    } else if (comm = "PWG") {

    } else if (comm = "QUT") {

    } else if (comm = "REV") {

    } else if (comm = "GSB") {

    } else if (comm = "GHL") {

    } else if (comm = "STA") {

    } else {
        throw invalid_argument("Invalid command");
    }
    return;
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