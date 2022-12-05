#include <unistd.h>
#include <netdb.h>
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <cctype>
#include <vector>
// #include <stdio.h>
// #include <sys/socket.h>

using namespace std;

#define GN 0    // TODO: change group number

// ======================================== Declarations ===========================================

struct addrinfo *addr_udp, *addr_tcp;   // addrinfo structs for UDP and TCP connections
string PLID;                            // Player ID
bool game = false;                      // Game status
vector<char> stateWord;                 // Current state of the word
int numLetters, maxErrors;              // Number of trials, number of letters in the word
int numErrors, numTrials;               // Max number of errors allowed, number of errors made

// Get the IP address of the given game server
addrinfo* get_server_address(string gsIP, string gsPort, string prot = "udp");
// Create sockets using IPv4 and the UDP or TCP protocol
int create_socket(string prot = "udp");

// Function that operates the commands
void handle_command(string comm, string arg);

// Sends a message to the server using the given socket
string request(int sock, addrinfo *server, string req);

// Checks if the given string is composed of
bool is_valid_port(const string &arg);      // digits only and is between 1024 and 65535
bool is_valid_plid(const string &arg);      // digits only and is of length 6
bool is_valid_letter(const string &arg);    // letters only and is of length 1
bool is_valid_word(const string &arg);      // letters only and is of length 1 or more

// Handle the responses from the game server
void handle_reply_start(string rep, string arg);
void handle_reply_play(string rep, string arg);
void handle_reply_guess(string rep, string arg);
void handle_reply_scoreboard(string rep, string arg);
void handle_reply_quit(string rep, string arg);
void handle_reply_reveal(string rep, string arg);

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
        if (!strcmp(argv[i], "-p")) {
            if (!is_valid_port(argv[i + 1]))
                throw invalid_argument("Invalid port number. Must be between 1024 and 65535.");
            else
                gsPort = argv[i + 1];
        }
    }
    // Get the Game Server addresses for UDP and TCP connections
    addr_udp = get_server_address(gsIP, gsPort);
    addr_tcp = get_server_address(gsIP, gsPort, "tcp");

    // Listen for instructions from the Player
    while (true) {
        getline(cin, line);
        stringstream(line) >> comm >> arg;
        handle_command(comm, arg);
    }
    // Forget the game server addresses and return
    freeaddrinfo(addr_udp);
    freeaddrinfo(addr_tcp);
    return 0;
}

// ==================================== Auxiliary Functions ========================================

addrinfo* get_server_address(string gsIP, string gsPort, string prot) {
    int errcode;
    struct addrinfo hints, *server;
    int type = (prot == "tcp") ? SOCK_STREAM : SOCK_DGRAM;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = type;
    errcode = getaddrinfo(gsIP.c_str(), gsPort.c_str(), &hints, &server);
    if (errcode != 0)
        throw runtime_error("Error getting address info.");
    return server;
}

int create_socket(string prot) {
    int type = (prot == "tcp") ? SOCK_STREAM : SOCK_DGRAM;
    int sock = socket(AF_INET, type, 0);
    if (sock == -1)
        throw runtime_error("Error opening " + prot + " socket.");
    return sock;
}

void handle_command(string comm, string arg) {
    string req, rep;
    int sock;
    // Start
    if (comm == "start" || comm == "sg") {
        if (!is_valid_plid(arg))    // Exception
            cout << "Invalid PLID." << endl;
        else {
            req = "SNG " + arg + "\n";
            sock = create_socket();
            rep = request(sock, addr_udp, req);
            close(sock);
            handle_reply_start(rep, arg);
        }
    }
    // Play
    else if (comm == "play" || comm == "pl") {
        if (!is_valid_letter(arg))  // Exception
            cout << "Invalid letter." << endl;
        else {
            req = "PLG " + PLID + " " + arg + " " + to_string(numTrials + 1) + "\n";
            sock = create_socket();
            rep = request(sock, addr_udp, req);
            close(sock);
            handle_reply_play(rep, arg);
        }
    }
    // Guess
    else if (comm == "guess" || comm == "gw") {
        if (!is_valid_word(arg))    // Exception
            cout << "Invalid word." << endl;
        else {
            req = "PWG " + PLID + " " + arg + " " + to_string(numTrials + 1) + "\n";
            sock = create_socket();
            rep = request(sock, addr_udp, req);
            close(sock);
            handle_reply_guess(rep, arg);
        }
    }
    // TODO: Scoreboard
    else if (comm == "scoreboard" || comm == "sb") {
        req = "GSB\n";
        sock = create_socket("tcp");
        rep = request(sock, addr_tcp, req);
        close(sock);
        handle_reply_scoreboard(rep, arg);
    }
    // TODO: Hint
    else if (comm == "hint" || comm == "h") {
    }
    // TODO: State
    else if (comm == "state" || comm == "st") {
    }
    // Quit
    else if (comm == "quit") {
        req = "QUT " + PLID + "\n";
        sock = create_socket();
        rep = request(sock, addr_udp, req);
        close(sock);
        handle_reply_quit(rep, arg);
    }
    // Exit
    else if (comm == "exit") {
        req = "QUT " + PLID + "\n";
        sock = create_socket();
        rep = request(sock, addr_udp, req);
        close(sock);
        handle_reply_quit(rep, arg);
        cout << "Exiting..." << endl;
        exit(0);
    }
    // Reveal
    else if (comm == "rev") {
        req = "REV " + PLID + "\n";
        sock = create_socket();
        rep = request(sock, addr_udp, req);
        close(sock);
        handle_reply_reveal(rep, arg);
    }
    else
        cout << "Invalid command." << endl;
    return;
}

bool is_valid_port(const string &arg) {
    return all_of(arg.begin(), arg.end(), ::isdigit) && (stoi(arg) >= 1024) && (stoi(arg) <= 65536);
}

bool is_valid_plid(const string &arg) {
    return all_of(arg.begin(), arg.end(), ::isdigit) && (arg.length() == 6);
}

bool is_valid_letter(const string &arg) {
    return (isalpha(arg[0]) && arg.length() == 1);
}

bool is_valid_word(const string &arg) {
    return all_of(arg.begin(), arg.end(), ::isalpha) && (arg.length() > 0);
}

string request(int sock, addrinfo *addr, string req) {
    socklen_t addrlen;
    struct sockaddr_in address;
    char buffer[128]; // TODO: what size should this be?
    
    // UDP connection request
    if (addr->ai_socktype == SOCK_DGRAM) {
        // Connect and send a message
        if (sendto(sock, req.c_str(), req.length(), 0, addr->ai_addr, addr->ai_addrlen) == -1)
            throw runtime_error("Error sending UDP request message.");
        // Receive a message
        addrlen = sizeof(address);
        if (recvfrom(sock, buffer, 128, 0, (struct sockaddr*) &address, &addrlen) == -1)
            throw runtime_error("Error receiving UDP reply message.");
    }
    // TCP connection request
    else if (addr->ai_socktype == SOCK_STREAM) {
        // Connect to the server
        if (connect(sock, addr->ai_addr, addr->ai_addrlen) == -1)
            throw runtime_error("Error connecting to server.");
        // Send a message
        if (write(sock, req.c_str(), req.length()) == -1)
            throw runtime_error("Error sending TCP request message.");
        // Receive a message
        if (read(sock, buffer, 128) == -1)
            throw runtime_error("Error receiving TCP reply message.");
    }
    else
        throw runtime_error("Invalid socket type.");
    return buffer;
}

void handle_reply_start(string rep, string arg) {
    // (RSG status [n_letters max_errors])
    string type, status;
    stringstream ss(rep);
    ss >> type >> status;
    
    // "NOK": The player has an ongoing game
    if (strcmp(status.c_str(), "NOK") == 0)
        cout << "There is an ongoing game for this Player." << endl;
    // "OK": The letter guess was successful
    else {
        ss >> numLetters >> maxErrors;
        PLID = arg; game = true;
        stateWord.assign(numLetters, '_');
        numErrors = 0; numTrials = 0;
        // Print the response message
        string res = "New game started. Guess " + to_string(numLetters) + " letter word: ";
        for (char i: stateWord)
            res += string(1, i) + " ";
        cout << res << endl;
    }
    return;
}

void handle_reply_play(string rep, string arg) {
    // (RLG status trial [n pos*])
    string type, status, res;
    stringstream ss(rep);
    ss >> type >> status;

    // "ERR": There is no ongoing game or the syntax of the request or PLID are invalid
    if (strcmp(status.c_str(), "ERR") == 0)
        cout << "The request or the PLID are invalid or there is no ongoing game for this Player." << endl;
    else {
    ss >> numTrials;
        // "OK": The letter guess was successful
        if (strcmp(status.c_str(), "OK") == 0) {
            int n, pos;
            ss >> n;
            for (int i = 0; i < n; i++) {
                ss >> pos;
                stateWord[pos - 1] = arg[0];
            }
            for (char i: stateWord)
                res += string(1, i) + " ";
            cout << "Correct letter!\t\tWord: " << res << endl;
        }
        // "WIN": The letter guess completes the word
        else if (strcmp(status.c_str(), "WIN") == 0) {
            game = false;
            for (int i = 0; i < numLetters; i++) {
                if (stateWord[i] == '_')
                    stateWord[i] = arg[0];
            }
            for (char i: stateWord)
                res += string(1, i) + " ";
            cout << "You won!\t\tWord: " << res << endl;
        }
        // "DUP": The letter guess was already made
        else if (strcmp(status.c_str(), "DUP") == 0) {
            for (char i: stateWord)
                res += string(1, i) + " ";
            cout << "Duplicate letter!\tWord: " << res << endl;
        }
        // "NOK": The letter guess was not successful
        else if (strcmp(status.c_str(), "NOK") == 0) {
            numErrors++;
            for (char i: stateWord)
                res += string(1, i) + " ";
            cout << "Wrong letter!\t\tWord: " << res << endl;
        }
        // "OVR": The letter guess was not successful and the game is over
        else if (strcmp(status.c_str(), "OVR") == 0) {
            numErrors++; game = false;
            cout << "You lost!" << endl;
        }
        // "INV": The trial number is invalid
        else if (strcmp(status.c_str(), "INV") == 0)
            cout << "Invalid trial number." << endl;
    }
    return;
}

void handle_reply_guess(string rep, string arg) {
    // (RWG status trial)
    string type, status;
    stringstream ss(rep);
    ss >> type >> status;
    
    // "ERR": There is no ongoing game or the syntax of the request or PLID are invalid
    if (strcmp(status.c_str(), "ERR") == 0)
        cout << "The request or the PLID are invalid or there is no ongoing game for this Player." << endl;
    else {
    ss >> numTrials;
        // "WIN": The word guess was successful
        if (strcmp(status.c_str(), "WIN") == 0) {
            game = false;
            string res;
            for (char i: arg)
                res += string(1, i) + " ";
            cout << "You won!\t\tWord: " << res << endl;
        }
        // "NOK": The word guess was not successful
        else if (strcmp(status.c_str(), "NOK") == 0) {
            numErrors++;
            string res;
            for (char i: stateWord)
                res += string(1, i) + " ";
            cout << "Wrong guess!\t\tWord: " << res << endl;
        }
        // "OVR": The word guess was not successful and the game is over
        else if (strcmp(status.c_str(), "OVR") == 0) {
            numErrors++; game = false;
            cout << "You lost!" << endl;
        }
        // "INV": The trial number is invalid
        else if (strcmp(status.c_str(), "INV") == 0)
            cout << "Invalid trial number." << endl;
    }
    return;
}

void handle_reply_scoreboard(string rep, string arg) {
    // (RSB status [Fname Fsize Fdata])
    string type, status;
    stringstream ss(rep);
    ss >> type >> status;
    
    // "EMPTY": The scoreboard is empty
    if (strcmp(status.c_str(), "EMPTY") == 0)
        cout << "The scoreboard is still empty. No game was yet won by any player." << endl;
    // "OK": The scoreboard is not empty
    else {
        string fname;
        ssize_t fsize, fdata;
        ss >> fname >> fsize >> fdata;
        cout << "Scoreboard" << endl << fdata;
    }
    return;
}

void handle_reply_quit(string rep, string arg) {
    // Handle reply to a quit request (RQT status)
    string type, status;
    stringstream(rep) >> type >> status;
    
    // "ERR": There is no ongoing game for this Player
    if (strcmp(status.c_str(), "ERR") == 0)
        cout << "There is no ongoing game for this Player." << endl;
    // "OK": There was an ongoing game and it was successfully quit
    else {
        game = false; // TODO: do we have to close TCP connections on this side?
        cout << "Game successfully quit." << endl;
    }
    return;
}

void handle_reply_reveal(string rep, string arg) {
    // (RRV word/status)
    string type, status;
    stringstream(rep) >> type >> status;
    
    // "ERR": There is no ongoing game
    if (strcmp(status.c_str(), "ERR") == 0)
        cout << "There is no ongoing game for this Player." << endl;
    // "word": The word was successfully revealed
    else {
        game = false;
        string res;
        for (char i: status)
            res += string(1, i) + " ";
        cout << "You have revealed the word: " << res << endl;
        int sock = create_socket();
        rep = request(sock, addr_udp, "QUT " + PLID + "\n");
        close(sock);
        handle_reply_quit(rep, arg);
    }
    return;
}