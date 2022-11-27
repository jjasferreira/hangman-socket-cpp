#include <unistd.h>
#include <netdb.h>
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <cctype>
#include <list>

using namespace std;

#define GN 0    // TODO: change group number

// ======================================== Declarations ===========================================

// Player ID, game status and number of trials, number of letters in the word, maximum number of errors allowed and number of errors made
string PLID;
bool game = false;
int numLetters, maxErrors;
int numErrors;
int numTrials;

// Get the IP address of the given game server
addrinfo* get_server_address(string gsIP, string gsPort);

// Create a socket using IPv4 and the UDP protocol
int create_udp_socket();

// Close the socket
void close_udp_socket(int sock);

// Function that operates the commands
void handle_command(string comm, string arg, int sock, addrinfo *server);

// Checks if the given string is composed of digits only and is of length 6
bool is_valid_plid(const string &arg);

// Request the game server to start the game
string start(int sock, addrinfo *server, string arg);

// Checks if the given string is composed of letters only and is of length 1
bool is_valid_letter(const string &arg);

// Request the game server to play a letter
string play(int sock, addrinfo *server, string arg);

// ============================================ Main ===============================================

int main(int argc, char *argv[]) {

    // Default Game Server's IP address and Port
    string gsIP = "";
    string gsPort = to_string(58000 + GN);

    // Input line, command and arguments
    string line, comm, arg;

    // Overwrite IP and Port ([-n gsIP] [-p gsPort])
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
        stringstream(line) >> comm >> arg;
        int sock = create_udp_socket();
        cout << "Command: " << comm << " arg:" << arg << endl; //TODO: remove
        handle_command(comm, arg, sock, server);
        close_udp_socket(sock);
    }

    // Forget the game server address
    freeaddrinfo(server);

    return 0;
}

// ==================================== Auxiliary Functions ========================================

addrinfo* get_server_address(string gsIP, string gsPort) {
    int errcode;
    struct addrinfo hints, *server;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    //TODO: "tejo.tecnico.ulisboa.pt", "58011" remove this line
    errcode = getaddrinfo(gsIP.c_str(), gsPort.c_str(), &hints, &server);
    if (errcode != 0)
        throw runtime_error("Error getting address info");
    return server;
}

int create_udp_socket() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
        throw runtime_error("Error opening socket");
    return sock;
}

void close_udp_socket(int sock) {
    close(sock);
}

void handle_command(string comm, string arg, int sock, addrinfo *server) {
    // Start command
    if (comm == "start" || comm == "sg") {
        // Exception
        if (!is_valid_plid(arg))
            throw invalid_argument("Invalid PLID");
        else {
            if (strcmp(start(sock, server, arg), "NOK") == 0)
                throw runtime_error("Error starting game");
        }
    }
    // Play command
    else if (comm == "play" || comm == "pl") {
        // Exceptions
        if (!game)
            throw runtime_error("Player has not started a game");
        else if (!is_valid_letter(arg))
            throw invalid_argument("Invalid Letter");
        else {
            rep = play(sock, server, arg);
            handle_reply_play(rep);
        }
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
    return;
}

bool is_valid_plid(const string &arg) {
    return all_of(arg.begin(), arg.end(), ::isdigit) && (arg.length() == 6);
}

string start(int sock, addrinfo *server, string arg) {
    ssize_t n;
    socklen_t addrlen;
    struct sockaddr_in addr;
    char buffer[128]; // TODO: what size should this be?

    // Send a message to the server (SNG PLID)
    string req = "SNG " + PLID + "\n";
    n = sendto(sock, req.c_str(), 11, 0, server->ai_addr, server->ai_addrlen);
    if (n == -1)
        throw runtime_error("Error sending start message");

    // Receive a message from the server
    addrlen = sizeof(addr);
    /*n = recvfrom(sock, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
    if (n == -1)
        throw runtime_error("Error receiving start confirmation");*/

    // Set PLID and other game variables and return status
    PLID = arg;
    game = true;
    numErrors = 0;
    numTrials = 0;
    strcpy(buffer, "RSG OK 10 7\n"); //TODO: remove this line
    string type, status;
    stringstream(buffer) >> type >> status >> numLetters >> maxErrors;
    return status;
}

bool is_valid_letter(const string &arg) {
    return (isalpha(arg[0]) && arg.length() == 1);
}

string play(int sock, addrinfo *server, string arg) {
    ssize_t n;
    socklen_t addrlen;
    struct sockaddr_in addr;
    char buffer[128]; // TODO: what size should this be?

    // Send a message to the server (PLG PLID letter trial)
    string req = "PLG " + PLID + " " + arg + " " + to_string(numTrials) + "\n";
    n = sendto(sock, req.c_str(), 16, 0, server->ai_addr, server->ai_addrlen);
    if (n == -1)
        throw runtime_error("Error sending play message");

    // Receive a message from the server
    addrlen = sizeof(addr);
    /*n = recvfrom(sock, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
    if (n == -1)
        throw runtime_error("Error receiving play confirmation");*/
 
    // Handle the reply message received from the Game Server
    strcpy(buffer, "RLG OK 1 2 5 8\n"); //TODO: remove this line
    return buffer;
}

void handle_reply_play(string rep) {
    // Handle reply to a play request (RLG status trial [n pos*])
    string type, status;
    stringstream(rep) >> type >> status;
    
    // "ERR": The syntax of the request is invalid
    if strcmp(status, "ERR") == 0 {
        cout << "The PLID is invalid or there is no ongoing game for this Player" << endl;
        return;
    }

    // Update the number of trials
    stringstream(rep) >> numTrials;

    // "OK": The letter guess was successful
    if strcmp(status, "OK") == 0 {
        int n, pos;
        list<int> listPos;
        stringstream(buffer) >> n;
        for (int i = 0; i < n; i++) {
            stringstream(buffer) >> pos;
            listPos.push_back(pos);
        }
    }
    // "WIN": The letter guess completes the word
    else if strcmp(status, "WIN") == 0
        game = false;
    // "DUP": The letter guess was already made
    else if strcmp(status, "DUP") == 0 {
        // TOD0: what to do?
    }
    // "NOK": The letter guess was not successful
    else if strcmp(status, "NOK") == 0
        numErrors++;
    // "OVR": The letter guess was not successful and the game is over
    else if strcmp(status, "OVR") == 0 {
        numErrors++;
        game = false;
    }
    // "INV": The trial number is invalid
    else if strcmp(status, "INV") == 0 {
        cout << "Invalid trial number" << endl;
    }
    return;
}