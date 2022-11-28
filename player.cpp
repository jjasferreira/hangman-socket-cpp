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

using namespace std;

#define GN 0    // TODO: change group number

// ======================================== Declarations ===========================================

// Player ID, game status and number of trials, number of letters in the word, maximum number of errors allowed and number of errors made
string PLID;
bool game = false;
vector<char> stateWord;
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

// Sends a message to the server using the given socket
string request(int sock, addrinfo *server, string req);

// Checks if the given string is composed of
bool is_valid_plid(const string &arg);      // digits only and is of length 6
bool is_valid_letter(const string &arg);    // letters only and is of length 1
bool is_valid_word(const string &arg);    // letters only and is of length 1 or more

// Handle the responses from the game server
void handle_reply_start(string rep, string arg);
void handle_reply_play(string rep, string arg);
void handle_reply_guess(string rep, string arg);

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
        handle_command(comm, arg, sock, server);
        close_udp_socket(sock);
    }

    // Forget the game server address and return
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
    string req, rep;
    // Start
    if (comm == "start" || comm == "sg") {
        if (!is_valid_plid(arg))    // Exception
            cout << "Invalid PLID" << endl;
        else {
            req = "SNG " + arg + "\n";
            rep = request(sock, server, req);
            handle_reply_start(rep, arg);
        }
    }
    // Play
    else if (comm == "play" || comm == "pl") {
        if (!is_valid_letter(arg))  // Exception
            cout << "Invalid letter" << endl;
        else {
            req = "PLG " + PLID + " " + arg + " " + to_string(++numTrials) + "\n";
            rep = request(sock, server, req);
            handle_reply_play(rep, arg);
        }
    }
    // Guess
    else if (comm == "guess" || comm == "gw") {
        if (!is_valid_word(arg))    // Exception
            cout << "Invalid word" << endl;
        else {
            req = "PWG " + PLID + " " + arg + " " + to_string(++numTrials) + "\n";
            rep = request(sock, server, req);
            handle_reply_guess(rep, arg);
        }
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
    return;
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

string request(int sock, addrinfo *server, string req) {
    ssize_t n;
    socklen_t addrlen;
    struct sockaddr_in addr;
    char buffer[128]; // TODO: what size should this be?

    // Send a message to the server
    n = sendto(sock, req.c_str(), req.length(), 0, server->ai_addr, server->ai_addrlen);
    if (n == -1)
        throw runtime_error("Error sending play message");

    // Receive a message from the server
    addrlen = sizeof(addr);
    n = recvfrom(sock, buffer, 128, 0, (struct sockaddr*) &addr, &addrlen);
    if (n == -1)
        throw runtime_error("Error receiving play confirmation");
    
    return buffer;
}

void handle_reply_start(string rep, string arg) {
    // Handle reply to a start request (RSG status [n_letters max_errors])
    string type, status;
    stringstream ss(rep);
    ss >> type >> status;
    
    // "NOK": The player has an ongoing game
    if (strcmp(status.c_str(), "NOK") == 0)
        cout << "There is an ongoing game for this Player" << endl;
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
    // Handle reply to a play request (RLG status trial [n pos*])
    string type, status;
    stringstream ss(rep);
    ss >> type >> status;

    // "ERR": There is no ongoing game or the syntax of the request or PLID are invalid
    if (strcmp(status.c_str(), "ERR") == 0)
        cout << "The request or the PLID are invalid or there is no ongoing game for this Player" << endl;
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
            string res = "Word: ";
            for (char i: stateWord)
                res += string(1, i) + " ";
            cout << "Correct guess! " << res << endl;
        }
        // "WIN": The letter guess completes the word
        else if (strcmp(status.c_str(), "WIN") == 0) {
            game = false;
            cout << "You won!" << endl;
        }
        // "DUP": The letter guess was already made
        else if (strcmp(status.c_str(), "DUP") == 0)
            cout << "Duplicate letter" << endl;
        // "NOK": The letter guess was not successful
        else if (strcmp(status.c_str(), "NOK") == 0) {
            numErrors++;
            cout << "Wrong letter" << endl;
        }
        // "OVR": The letter guess was not successful and the game is over
        else if (strcmp(status.c_str(), "OVR") == 0) {
            numErrors++; game = false;
            cout << "You lost!" << endl;
        }
        // "INV": The trial number is invalid
        else if (strcmp(status.c_str(), "INV") == 0)
            cout << "Invalid trial number" << endl;
    }
    return;
}

void handle_reply_guess(string rep, string arg) {
    // Handle reply to a guess request (RWG status trial)
    string type, status;
    stringstream ss(rep);
    ss >> type >> status;
    
    // "ERR": There is no ongoing game or the syntax of the request or PLID are invalid
    if (strcmp(status.c_str(), "ERR") == 0)
        cout << "The request or the PLID are invalid or there is no ongoing game for this Player" << endl;
    else {
    ss >> numTrials;
        // "WIN": The word guess was successful
        if (strcmp(status.c_str(), "WIN") == 0) {
            game = false;
            cout << "You won!" << endl;
        }
        // "NOK": The word guess was not successful
        else if (strcmp(status.c_str(), "NOK") == 0) {
            numErrors++;
            cout << "Wrong word" << endl;
        }
        // "OVR": The word guess was not successful and the game is over
        else if (strcmp(status.c_str(), "OVR") == 0) {
            numErrors++; game = false;
            cout << "You lost!" << endl;
        }
        // "INV": The trial number is invalid
        else if (strcmp(status.c_str(), "INV") == 0)
            cout << "Invalid trial number" << endl;
    }
    return;
}