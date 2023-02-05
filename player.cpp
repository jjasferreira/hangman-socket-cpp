#include "auxiliary.h"

// ======================================== Declarations ===========================================

string progName;                    // Program name
struct addrinfo *addrUDP, *addrTCP; // Addrinfo structs for UDP and TCP connections
string PLID;                        // Player ID
bool game = false;                  // Game status
bool id = false;                    // Id status
vector<char> wordProgress;          // Current state of the word
int numLetters, maxErrors;          // Number of trials, number of letters in the word
int numErrors, numTrials;           // Max number of errors allowed, number of errors made

// Print the messages to the terminal
void print_welcome_message();
void print_word_progress();

// Function that operates the commands
void handle_command(string line);

// Handle the different responses from the game server
void handle_reply_start(string rep, string arg);
void handle_reply_play(string rep, string arg);
void handle_reply_guess(string rep, string arg);
void handle_reply_scoreboard(int sock);
void handle_reply_hint(int sock);
void handle_reply_state(int sock);
void handle_reply_quit(string rep);
void handle_reply_reveal(string rep);

// Handle the signals from the terminal
void handle_signal_player(int sig);

// ============================================ Main ===============================================

int main(int argc, char *argv[]) {
    // Default Game Server's IP address and Port
    string gsIP = "localhost";
    string gsPort = to_string(58000 + GN);
    // Input line, command and arguments
    string line, comm, arg;

    // Define signal handler to safely exit the program
    if (signal(SIGINT, handle_signal_player) == SIG_ERR)
        throw runtime_error("Error defining signal handler");

    // Overwrite IP and Port ([-n gsIP] [-p gsPort])
    progName = argv[0];
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
        handle_command(line);
    }
    // Forget the game server addresses and return
    delete addrUDP;
    delete addrTCP;
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
    cout << "state\t\tDisplay game state\t(alias: st)" << endl;
    cout << "reveal\t\tReveal the word\t\t(alias: rev)" << endl << endl;
    cout << "quit\t\tQuit game" << endl;
    cout << "exit\t\tExit application" << endl;
}

void print_word_progress() {
    if (game) {
        string res = "Word progress: ";
        for (char i: wordProgress)
            res += string(1, i) + " ";
        cout << endl << res << endl;
        cout << "Current trial: " << numTrials << endl;
        cout << "Number of errors: " << numErrors << "/" << maxErrors << endl << endl;
    }
}

void handle_command(string line) {
    string comm, arg, req, rep;
    stringstream ss(line);
    ss >> comm;
    int sock;

    // Start
    if (comm == "start" || comm == "sg") {
        ss >> arg;
        if (!is_valid_plid(arg))    // Exception
            cout << "Invalid PLID." << endl << endl;
        else {
            req = "SNG " + arg + "\n" + "\0";
            sock = create_socket(addrUDP, progName);
            write_to_socket(sock, addrUDP, req);
            rep = read_from_socket(sock);
            close(sock);
            handle_reply_start(rep, arg);
        }
    }
    // Play
    else if (comm == "play" || comm == "pl") {
        ss >> arg;
        if (!game)  // Exception
            cout << "There is no ongoing game for this Player." << endl << endl;
        else if (!is_valid_letter(arg)) // Exception
            cout << "Invalid letter." << endl << endl;
        else {
            arg[0] = tolower(arg[0]);
            cout << arg << endl;
            req = "PLG " + PLID + " " + arg + " " + to_string(numTrials + 1) + "\n";
            sock = create_socket(addrUDP, progName);
            write_to_socket(sock, addrUDP, req);
            rep = read_from_socket(sock);
            close(sock);
            handle_reply_play(rep, arg);
        }
    }
    // Guess
    else if (comm == "guess" || comm == "gw") {
        ss >> arg;
        if (!game)  // Exception
            cout << "There is no ongoing game for this Player." << endl << endl;
        if (!is_valid_word(arg))    // Exception
            cout << "Invalid word." << endl << endl;
        else {
            transform(arg.begin(), arg.end(), arg.begin(), ::tolower);
            req = "PWG " + PLID + " " + arg + " " + to_string(numTrials + 1) + "\n";
            sock = create_socket(addrUDP, progName);
            write_to_socket(sock, addrUDP, req);
            rep = read_from_socket(sock);
            close(sock);
            handle_reply_guess(rep, arg);
        }
    }
    // Scoreboard
    else if (comm == "scoreboard" || comm == "sb") {
        req = "GSB\n";
        sock = create_socket(addrTCP, progName);
        write_to_socket(sock, addrTCP, req);
        handle_reply_scoreboard(sock);
        close(sock);
    }
    // Hint
    else if (comm == "hint" || comm == "h") {
        if (!game)  // Exception
            cout << "There is no ongoing game for this Player." << endl << endl;
        else {
            req = "GHL " + PLID + "\n";
            sock = create_socket(addrTCP, progName);
            write_to_socket(sock, addrTCP, req);
            handle_reply_hint(sock);
            close(sock);
        }
    }
    // State
    else if (comm == "state" || comm == "st") {
        if (!id)  // Exception
            cout << "The PLID is still unknown to the server." << endl << endl;
        else {
            req = "STA " + PLID + "\n";
            sock = create_socket(addrTCP, progName);
            write_to_socket(sock, addrTCP, req);
            handle_reply_state(sock);
            close(sock);
        }
    }
    // Quit
    else if (comm == "quit") {
        req = "QUT " + PLID + "\n";
        sock = create_socket(addrUDP, progName);
        write_to_socket(sock, addrUDP, req);
        rep = read_from_socket(sock);
        close(sock);
        handle_reply_quit(rep);
    }
    // Exit
    else if (comm == "exit")
        handle_signal_player(SIGINT);
    // Reveal
    else if (comm == "rev") {
        if (!game)  // Exception
            cout << "There is no ongoing game for this Player." << endl << endl;
        else {
            req = "REV " + PLID + "\n";
            sock = create_socket(addrUDP, progName);
            write_to_socket(sock, addrUDP, req);
            rep = read_from_socket(sock);
            close(sock);
            handle_reply_reveal(rep);
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
    
    // "ERR": The PLID or the syntax was invalid
    if (status == "ERR")
        cout << "The PLID or the syntax of the request was invalid." << endl << endl;
    // "NOK": The player has an ongoing game
    else if (status == "NOK") {
        id = true;
        cout << "There is an ongoing game for this Player." << endl << endl;
    }
    // "OK": The player has started a game
    else if (status == "OK") {
        id = true;
        ss >> numLetters >> maxErrors;
        PLID = arg; game = true;
        wordProgress.assign(numLetters, '_');
        numErrors = 0; numTrials = 0;
        // Print the response message
        cout << "New game started successfully." << endl;
        print_word_progress();
    }
    else    // Exception
        cout << "Undefined reply from the server: " << rep << endl << endl;
}

void handle_reply_play(string rep, string arg) {
    // (RLG status trial [n pos*])
    string type, status;
    stringstream ss(rep);
    ss >> type >> status;

    // "ERR": There is no ongoing game or the syntax of the request or PLID are invalid
    if (status == "ERR")
        cout << "The PLID or the request was invalid or there is no ongoing game for this Player." << endl << endl;
    else {
        ss >> numTrials;
        // "OK": The letter guess was successful
        if (status == "OK") {
            int numPos, pos;
            ss >> numPos;
            for (int i = 0; i < numPos; i++) {
                ss >> pos;
                wordProgress[pos - 1] = arg[0];
            }
            cout << "Correct letter!" << endl;
            print_word_progress();
        }
        // "WIN": The letter guess completes the word
        else if (status == "WIN") {
            for (int i = 0; i < numLetters; i++) {
                if (wordProgress[i] == '_')
                    wordProgress[i] = arg[0];
            }
            cout << "You won!" << endl;
            print_word_progress();
            game = false;
        }
        // "DUP": The letter guess was already made
        else if (status == "DUP") {
            cout << "Duplicate letter!" << endl;
            print_word_progress();
        }
        // "NOK": The letter guess was not successful
        else if (status == "NOK") {
            numErrors++;
            cout << "Wrong letter!" << endl;
            print_word_progress();
        }
        // "OVR": The letter guess was not successful and the game is over
        else if (status == "OVR") {
            numErrors++;
            game = false;
            cout << "You lost!" << endl;
        }
        // "INV": The trial number is invalid
        else if (status == "INV")
            cout << "Invalid trial number." << endl;
        else    // Exception
            cout << "Undefined reply from the server: " << rep << endl << endl;
    }
}

void handle_reply_guess(string rep, string arg) {
    // (RWG status trial)
    string type, status;
    stringstream ss(rep);
    ss >> type >> status;
    
    // "ERR": There is no ongoing game or the syntax of the request or PLID are invalid
    if (status == "ERR")
        cout << "The PLID or the request was invalid or there is no ongoing game for this Player." << endl << endl;
    else {
    ss >> numTrials;
        // "WIN": The word guess was successful
        if (status == "WIN") {
            for (int i = 0; i < numLetters; i++)
                wordProgress[i] = arg[i];
            cout << "You won!" << endl;
            print_word_progress();
            game = false;
        }
        // "DUP": The word guess was already made
        else if (status == "DUP") {
            cout << "Duplicate guess!" << endl;
            print_word_progress();
        }
        // "NOK": The word guess was not successful
        else if (status == "NOK") {
            numErrors++;
            cout << "Wrong guess!" << endl;
            print_word_progress();
        }
        // "OVR": The word guess was not successful and the game is over
        else if (status == "OVR") {
            numErrors++;
            game = false;
            cout << "You lost!" << endl;
        }
        // "INV": The trial number is invalid
        else if (status == "INV")
            cout << "Invalid trial number." << endl;
        else    // Exception
            cout << "Undefined reply from the server: " << rep << endl << endl;
    }
}

void handle_reply_scoreboard(int sock) {
    // (RSB status [Fname Fsize Fdata])
    string type, status, fname, fsize;
    ssize_t n; char c;

    // Get the type of message received
    while ((n = read(sock, &c, 1)) > 0 && c != ' ')
        type += c;
    if (type != "RSB") {    // Exception
        cout << "Unexpected reply from the server: " << type << " " << status << endl << endl;
        return;
    }
    // Get the status
    while ((n = read(sock, &c, 1)) > 0 && c != ' ' && c != '\n')
        status += c;
    // "EMPTY": The scoreboard is empty
    if (status == "EMPTY")
        cout << "The scoreboard is still empty. No game was yet won by any player." << endl << endl;
    // "OK": The scoreboard is not empty
    else if (status == "OK") {
        string res = read_to_file(sock, "print");
        if (res == "FNERR" || res == "FSERR")   // Exception
            return;
        stringstream(res) >> fname >> fsize;
        cout << "Saved scoreboard to file " << fname << " (" << fsize << " bytes)." << endl << endl;
    }
    else    // Exception
        cout << "Undefined reply from the server: " << type << " " << status << endl << endl;
}

void handle_reply_hint(int sock) {
    // (RHL status [Fname Fsize Fdata])
    string type, status, fname, fsize;
    ssize_t n; char c;

    // Get the type of message received
    while ((n = read(sock, &c, 1)) > 0 && c != ' ')
        type += c;
    if (type != "RHL") {    // Exception
        cout << "Unexpected reply from the server: " << type << " " << status << endl << endl;
        return;
    }
    // Get the status
    while ((n = read(sock, &c, 1)) > 0 && c != ' ' && c != '\n')
        status += c;
    // "NOK": There is no file to be sent
    if (status == "NOK")
        cout << "There is no file to be sent or there is some other problem." << endl << endl;
    // "OK": The hint is sent as an image file
    else if (status == "OK") {
        string res = read_to_file(sock);
        if (res == "FNERR" || res == "FSERR")   // Exception
            return;
        stringstream(res) >> fname >> fsize;
        cout << "Saved hint to file " << fname << " (" << fsize << " bytes)." << endl;
    }
    else    // Exception
        cout << "Undefined reply from the server: " << type << " " << status << endl << endl;
    print_word_progress();
}

void handle_reply_state(int sock) {
    // (RST status [Fname Fsize Fdata])
    string type, status, fname, fsize;
    ssize_t n; char c;

    // Get the type of message received
    while ((n = read(sock, &c, 1)) > 0 && c != ' ')
        type += c;
    if (type != "RST") {    // Exception
        cout << "Unexpected reply from the server: " << type << " " << status << endl << endl;
        return;
    }
    // Get the status
    while ((n = read(sock, &c, 1)) > 0 && c != ' ' && c != '\n')
        status += c;
    // "NOK": There are no games (active or finished)
    if (status == "NOK")
        cout << "There are no games (active or finished) for this Player." << endl << endl;
    // "ACT": There is an ongoing game. "FIN": There is no ongoing game. The file is sent
    else if (status == "ACT" || status == "FIN") {
        string res = read_to_file(sock, "print");
        if (res == "FNERR" || res == "FSERR")   // Exception
            return;
        stringstream(res) >> fname >> fsize;
        cout << "Saved state to file " << fname << " (" << fsize << " bytes)." << endl << endl;
        if (status == "ACT")
            print_word_progress();
        else if (status == "FIN")
            game = false;
    }
    else    // Exception
        cout << "Undefined reply from the server: " << type << " " << status << endl << endl;
}

void handle_reply_quit(string rep) {
    // (RQT status)
    string type, status;
    stringstream(rep) >> type >> status;
    
    // "ERR": The PLID or the syntax was invalid
    if (status == "ERR")
        cout << "The PLID or the syntax of the request was invalid." << endl << endl;
    else {
        game = false;
        // "OK": There was an ongoing game and it was successfully quit
        if (status == "OK")
            cout << "Game successfully quit." << endl << endl;
        // "NOK": There is no ongoing game for this Player
        else if (status == "NOK")
            cout << "There is no ongoing game for this Player." << endl << endl;
        else    // Exception
            cout << "Undefined reply from the server: " << rep << endl << endl;
    }
}

void handle_reply_reveal(string rep) {
    // (RRV word)
    string type, status;
    stringstream(rep) >> type >> status;
    
    // "ERR": There is no ongoing game or the syntax of the request or PLID are invalid
    if (status == "ERR")
        cout << "The PLID or the request was invalid or there is no ongoing game for this Player." << endl << endl;
    // "word": The word was successfully revealed
    else if (is_valid_word(status)) {
        for (int i = 0; i < numLetters; i++)
            wordProgress[i] = status[i];
        cout << "You have revealed the word." << endl;
        print_word_progress();
        game = false;
        int sock = create_socket(addrUDP, progName);
        write_to_socket(sock, addrUDP, "QUT " + PLID + "\n");
        rep = read_from_socket(sock);
        close(sock);
        handle_reply_quit(rep);
    }
    else    // Exception
        cout << "Undefined reply from the server: " << rep << endl << endl;
}

void handle_signal_player(int sig) {
    // Ctrl + C
    if (sig == SIGINT) {
        cout << endl;
        // If there is an ongoing game, quit it
        if (game) {
            string req = "QUT " + PLID + "\n";
            int sock = create_socket(addrUDP, progName);
            write_to_socket(sock, addrUDP, "QUT " + PLID + "\n");
            string rep = read_from_socket(sock);
            close(sock);
            handle_reply_quit(rep);
        }
        // Free the allocated memory for the addresses
        delete addrUDP;
        delete addrTCP;
        cout << "Exiting..." << endl << endl;
        exit(0);
    }
}