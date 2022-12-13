#include "common.h"

// ======================================== Declarations ===========================================

struct addrinfo *addrUDP, *addrTCP; // addrinfo structs for UDP and TCP connections
string PLID;                        // Player ID
bool game = false;                  // Game status
vector<char> wordProgress;          // Current state of the word
int numLetters, maxErrors;          // Number of trials, number of letters in the word
int numErrors, numTrials;           // Max number of errors allowed, number of errors made

// Print the messages to the terminal
void print_welcome_message();
void print_word_progress();

// Function that operates the commands
void handle_command(string comm, string arg);

// Handle the responses from the game server
void handle_reply_start(string rep, string arg);
void handle_reply_play(string rep, string arg);
void handle_reply_guess(string rep, string arg);
void handle_reply_scoreboard(string rep, string arg, int sock);
void handle_reply_hint(string rep, string arg, int sock);
void handle_reply_state(string rep, string arg, int sock);
void handle_reply_quit(string rep, string arg);
void handle_reply_reveal(string rep, string arg);

// ============================================ Main ===============================================

int main(int argc, char *argv[]) {

    // Default Game Server's IP address and Port
    string gsIP = "localhost";
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
    addrUDP = get_server_address(gsIP, gsPort);
    addrTCP = get_server_address(gsIP, gsPort, "tcp");

    // Listen for instructions from the Player
    print_welcome_message();
    while (true) {
        cout << "> ";
        getline(cin, line);
        stringstream(line) >> comm >> arg;
        handle_command(comm, arg);
    }
    // Forget the game server addresses and return
    freeaddrinfo(addrUDP);
    freeaddrinfo(addrTCP);
    return 0;
}

// ==================================== Auxiliary Functions ========================================

void print_welcome_message() {
    cout << endl << "Welcome to the Hangman Game! Available commands:" << endl;
    cout << "start <PLID>\tStart a new game\t(alias: sg)" << endl;
    cout << "play <letter>\tGuess a letter\t\t(alias: pl)" << endl;
    cout << "guess <word>\tGuess the word\t\t(alias: gw)" << endl;
    cout << "scoreboard\tDisplay scoreboard\t(alias: sb)" << endl;
    cout << "hint\t\tGet a hint for the word\t(alias: h)" << endl;
    cout << "quit\t\tQuit game" << endl;
    cout << "exit\t\tExit application" << endl;
    cout << "reveal\t\tReveal the word\t\t(alias: rv)" << endl << endl;
}

void print_word_progress() {
    string res = "Word progress: ";
    for (char i: wordProgress)
        res += string(1, i) + " ";
    cout << endl << res << endl;
    cout << "Current trial: " << numTrials << endl;
    cout << "Number of errors: " << numErrors << "/" << maxErrors << endl << endl;
}

void handle_command(string comm, string arg) {
    string req, rep;
    int sock;
    // Start
    if (comm == "start" || comm == "sg") {
        if (!is_valid_plid(arg))    // Exception
            cout << "Invalid PLID." << endl << endl;
        else {
            req = "SNG " + arg + "\n";
            sock = create_socket();
            rep = request(sock, addrUDP, req);
            close(sock);
            handle_reply_start(rep, arg);
        }
    }
    // Play
    else if (comm == "play" || comm == "pl") {
        if (!game) // Exception
            cout << "There is no ongoing game for this Player." << endl << endl;
        else if (!is_valid_letter(arg))  // Exception
            cout << "Invalid letter." << endl << endl;
        else {
            req = "PLG " + PLID + " " + arg + " " + to_string(numTrials + 1) + "\n";
            sock = create_socket();
            rep = request(sock, addrUDP, req);
            close(sock);
            handle_reply_play(rep, arg);
        }
    }
    // Guess
    else if (comm == "guess" || comm == "gw") {
        if (!game) // Exception
            cout << "There is no ongoing game for this Player." << endl << endl;
        if (!is_valid_word(arg))    // Exception
            cout << "Invalid word." << endl << endl;
        else {
            req = "PWG " + PLID + " " + arg + " " + to_string(numTrials + 1) + "\n";
            sock = create_socket();
            rep = request(sock, addrUDP, req);
            close(sock);
            handle_reply_guess(rep, arg);
        }
    }
    // Scoreboard
    else if (comm == "scoreboard" || comm == "sb") {
        req = "GSB\n";
        sock = create_socket("tcp");
        rep = request(sock, addrTCP, req);
        handle_reply_scoreboard(rep, arg, sock);
        close(sock);
    }
    // Hint
    else if (comm == "hint" || comm == "h") {
        if (!game) // Exception
            cout << "There is no ongoing game for this Player." << endl << endl;
        else {
            req = "GHL " + PLID + "\n";
            sock = create_socket("tcp");
            rep = request(sock, addrTCP, req);
            handle_reply_hint(rep, arg, sock);
            close(sock);
        }
    }
    // State
    else if (comm == "state" || comm == "st") {
        req = "STA " + PLID + "\n";
        sock = create_socket("tcp");
        rep = request(sock, addrTCP, req);
        handle_reply_state(rep, arg, sock);
        close(sock);
    }
    // Quit
    else if (comm == "quit") {
        req = "QUT " + PLID + "\n";
        sock = create_socket();
        rep = request(sock, addrUDP, req);
        close(sock);
        handle_reply_quit(rep, arg);
    }
    // Exit
    else if (comm == "exit") {
        req = "QUT " + PLID + "\n";
        sock = create_socket();
        rep = request(sock, addrUDP, req);
        close(sock);
        handle_reply_quit(rep, arg);
        cout << "Exiting..." << endl << endl;
        exit(0);
    }
    // Reveal
    else if (comm == "rev") {
        if (!game) // Exception
            cout << "There is no ongoing game for this Player." << endl << endl;
        else {
            req = "REV " + PLID + "\n";
            sock = create_socket();
            rep = request(sock, addrUDP, req);
            close(sock);
            handle_reply_reveal(rep, arg);
        }
    }
    else
        cout << "Invalid command." << endl << endl;
}

void handle_reply_start(string rep, string arg) {
    // (RSG status [n_letters max_errors])
    string type, status;
    stringstream ss(rep);
    ss >> type >> status;
    
    // "NOK": The player has an ongoing game
    if (strcmp(status.c_str(), "NOK") == 0)
        cout << "There is an ongoing game for this Player." << endl << endl;
    // "OK": The letter guess was successful
    else {
        ss >> numLetters >> maxErrors;
        PLID = arg; game = true;
        wordProgress.assign(numLetters, '_');
        numErrors = 0; numTrials = 0;
        // Print the response message
        cout << "New game started successfully." << endl;
        print_word_progress();
    }
}

void handle_reply_play(string rep, string arg) {
    // (RLG status trial [n pos*])
    string type, status;
    stringstream ss(rep);
    ss >> type >> status;

    // "ERR": There is no ongoing game or the syntax of the request or PLID are invalid
    if (strcmp(status.c_str(), "ERR") == 0)
        cout << "The request or the PLID are invalid or there is no ongoing game for this Player." << endl << endl;
    else {
    ss >> numTrials;
        // "OK": The letter guess was successful
        if (strcmp(status.c_str(), "OK") == 0) {
            int n, pos;
            ss >> n;
            for (int i = 0; i < n; i++) {
                ss >> pos;
                wordProgress[pos - 1] = arg[0];
            }
            cout << "Correct letter!" << endl;
            print_word_progress();
        }
        // "WIN": The letter guess completes the word
        else if (strcmp(status.c_str(), "WIN") == 0) {
            game = false;
            for (int i = 0; i < numLetters; i++) {
                if (wordProgress[i] == '_')
                    wordProgress[i] = arg[0];
            }
            cout << "You won!" << endl;
            print_word_progress();
        }
        // "DUP": The letter guess was already made
        else if (strcmp(status.c_str(), "DUP") == 0) {
            cout << "Duplicate letter!" << endl;
            print_word_progress();
        }
        // "NOK": The letter guess was not successful
        else if (strcmp(status.c_str(), "NOK") == 0) {
            numErrors++;
            cout << "Wrong letter!" << endl;
            print_word_progress();
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
}

void handle_reply_guess(string rep, string arg) {
    // (RWG status trial)
    string type, status;
    stringstream ss(rep);
    ss >> type >> status;
    
    // "ERR": There is no ongoing game or the syntax of the request or PLID are invalid
    if (strcmp(status.c_str(), "ERR") == 0)
        cout << "The request or the PLID are invalid or there is no ongoing game for this Player." << endl << endl;
    else {
    ss >> numTrials;
        // "WIN": The word guess was successful
        if (strcmp(status.c_str(), "WIN") == 0) {
            game = false;
            for (int i = 0; i < numLetters; i++)
                wordProgress[i] = arg[i];
            cout << "You won!" << endl;
            print_word_progress();
        }
        // "NOK": The word guess was not successful
        else if (strcmp(status.c_str(), "NOK") == 0) {
            numErrors++;
            cout << "Wrong guess!" << endl;
            print_word_progress();
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
}

void handle_reply_scoreboard(string rep, string arg, int sock) {
    // (RSB status [Fname Fsize Fdata])
    string type, status, res, fname, fsize;
    stringstream(rep) >> type >> status;
    
    // "EMPTY": The scoreboard is empty
    if (strcmp(status.c_str(), "EMPTY") == 0)
        cout << "The scoreboard is still empty. No game was yet won by any player." << endl << endl;
    // "OK": The scoreboard is not empty
    else {
        res = read_to_file(sock, "print");
        stringstream(res) >> fname >> fsize;
        cout << "Saved scoreboard to file " << fname << " (" << fsize << " bytes)." << endl << endl;
    }
}

void handle_reply_hint(string rep, string arg, int sock) {
    // (RHL status [Fname Fsize Fdata])
    string type, status, res, fname, fsize;
    stringstream(rep) >> type >> status;
    
    // "NOK": There is no file to be sent
    if (strcmp(status.c_str(), "NOK") == 0)
        cout << "There is no file to be sent or there is some other problem." << endl << endl;
    // "OK": The hint is sent as an image file
    else {
        res = read_to_file(sock);
        stringstream(res) >> fname >> fsize;
        cout << "Saved hint to file " << fname << " (" << fsize << " bytes)." << endl;
        print_word_progress();
    }
}

void handle_reply_state(string rep, string arg, int sock) {
    // (RST status [Fname Fsize Fdata])
    string type, status, res, fname, fsize;
    stringstream(rep) >> type >> status;
    
    // "NOK": There are no games (active or finished)
    if (strcmp(status.c_str(), "NOK") == 0)
        cout << "There are no games (active or finished) for this Player." << endl << endl;
    // "FIN": There is no ongoing game // "ACT": There is an ongoing game and the file is sent
    else {
        res = read_to_file(sock, "print");
        stringstream(res) >> fname >> fsize;
        cout << "Saved state to file " << fname << " (" << fsize << " bytes)." << endl << endl;
    }
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
        cout << "Game successfully quit." << endl << endl;
    }
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
        for (int i = 0; i < numLetters; i++)
            wordProgress[i] = status[i];
        cout << "You have revealed the word." << endl;
        print_word_progress();
        int sock = create_socket();
        rep = request(sock, addrUDP, "QUT " + PLID + "\n");
        close(sock);
        handle_reply_quit(rep, arg);
    }
}