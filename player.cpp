#include <unistd.h>
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

// Function that operates the commands
string handle_command(string comm, string args, string gsIP, int gsPort);

// ============================================ Main ===============================================

int main(int argc, char *argv[]) {

    // Game Server's IP address and Port
    string gsIP = "";
    int gsPort = 58000 + GN;

    // Player ID and Game status
    string PLID;
    bool game = false;

    // Input line, comm, and arguments
    string line, comm, args;

    // Set IP and Port ([-n gsIP] [-p gsPort])
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-n"))
            gsIP = argv[i + 1];
        if (!strcmp(argv[i], "-p"))
            gsPort = atoi(argv[i + 1]);
    }

    // Listen for instructions from the Player
    while (true) {
        getline(cin, line);
        stringstream(line) >> comm >> args;
        PLID = handle_command(comm, args, gsIP, gsPort);
    }

    // Number of letters in the word, Maximum number of errors allowed and number of errors made
    int numLetters;
    int maxErrors;
    int numErrors;

    // Talks to server and sets game to True if ID is valid
    game = true;

    return 0;
}

// ==================================== Auxiliary Functions ========================================

void connect_to_server(string gsIP, int gsPort) {
    int fd, errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints, *res;
    struct sockaddr_in addr;
    char buffer[128];

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) exit(1);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    errcode = getaddrinfo(gsIP, gsPort, &hints, &res);
    if (errcode != 0) exit(1);

    n = sendto(fd, "hello!\n", 7, 0, res->ai_addr, res->ai_addrlen);
    if (n == -1) exit(1);
}

bool is_valid_plid(const string &str) {
    return all_of(str.begin(), str.end(), ::isdigit) && (str.length() == 6);
}

string handle_command(string comm, string args, string gsIP, int gsPort) {
    if (comm == "start" || comm == "sg") {
        if (!is_valid_plid(args))
            cout << "Invalid PLID" << endl;
        else {
            // TODO: Send a message to the GS using UDP asking to start a new game. Provides PLID.
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
    else {
        cout << "Invalid command" << endl;
    }
}
