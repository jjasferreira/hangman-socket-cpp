#include "../common.h"

// ======================================== Declarations ===========================================

string progName;                    // Program name
struct addrinfo *addrUDP, *addrTCP; // Addrinfo structs for UDP and TCP connections
bool verbose = false;               // Verbose mode
pid_t pidChild;                     // Child process ID
string wordsFileName;               // File that stores the words and their hint file
ifstream wordsFile;                 // Input file stream for the words file

// Read a request from an UDP socket and from a certain Player address.
string read_from_socket_udp(int sock, addrinfo *addr, struct sockaddr_in *addrPlayer);
// Write a reply to an UDP socket using the Player address.
void write_to_socket_udp(int sock, struct sockaddr_in addrPlayer, string rep);
// Function that calls handlers depending on the UDP request
string handle_request_udp(string req);
// Handle the UDP requests from the Player
string handle_request_start(string PLID);
string handle_request_play(string PLID, string letter, string trial);
string handle_request_guess(string PLID, string guess, string trial);
string handle_request_quit(string PLID);
string handle_request_reveal(string PLID);

// Bind the TCP socket to the Game Server address and wait for connections
void setup_socket_tcp(int sock, addrinfo *addr);
// Function that calls handlers depending on the TCP request
void handle_request_tcp(string req, int nSock);
// Handle the TCP requests from the Player and send the reply
void handle_request_scoreboard(int nSock);
void handle_request_state(string PLID, int nSock);
void handle_request_hint(string PLID, int nSock);
// Send a file to the Player through the TCP socket
void send_file(int sock, string filePath);

// Handle the signals from the terminal
void handle_signal_gs(int sig);

// ============================================ Main ===============================================

int main(int argc, char *argv[]) {
    // Default Game Server's IP address and Port
    string gsIP = "localhost";
    string gsPort = to_string(58000 + GN);

    // Define signal handler to safely exit the program
    if (signal(SIGABRT, handle_signal_gs) == SIG_ERR)
        throw runtime_error("Error defining signal handler for SIGABRT.");
    if (signal(SIGSEGV, handle_signal_gs) == SIG_ERR)
        throw runtime_error("Error defining signal handler for SIGSEGV.");
    if (signal(SIGINT, handle_signal_gs) == SIG_ERR)
        throw runtime_error("Error defining signal handler for SIGINT.");

    // Check for input arguments (wordsFile [-p gsPort] [-v])
    if (argc < 2)   // Exception
        throw invalid_argument("No words file specified.");
    // Open the words file with the specified name
    else {
        wordsFileName = argv[1];
        wordsFile.open(wordsFileName);
        if (!wordsFile.is_open())   // Exception
            throw runtime_error("Error opening words file");
    }
    // Overwrite the default Port and verbose mode
    progName = argv[0];
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "-p"))
            gsPort = argv[i + 1];
        else if (!strcmp(argv[i], "-v"))
            verbose = true;
    }

    // Get the Game Server addresses for UDP and TCP connections
    addrUDP = get_server_address("self", gsPort);
    addrTCP = get_server_address("self", gsPort, "tcp");

    // Fork a new process
    pid_t pid = fork();
    // Child process listens to TCP connections
    if (pid == 0) {
        while (true) {
            int sock = create_socket(addrTCP, progName);
            setup_socket_tcp(sock, addrTCP);

            struct sockaddr_in addrPlayer;
            socklen_t addrLen = sizeof(addrPlayer);
            char buffer[1024];
            ssize_t n;
            while (true) {
                int nSock;
                // Accept connection to the server
                if ((nSock = accept(sock, (struct sockaddr*)&addrPlayer, &addrLen)) == -1)
                    throw runtime_error("Error accepting connection to server.");
                // Receive a message
                n = read(nSock, buffer, 1024);
                if (n == -1)
                    throw runtime_error("Error receiving TCP request message.");
                buffer[n] = '\0';
                // Print the IP address and Port of the Player, if in verbose mode
                if (verbose) {
                    char host[NI_MAXHOST], service[NI_MAXSERV];
                    if (getnameinfo((struct sockaddr*)&addrPlayer, addrLen, host, sizeof(host), service, sizeof(service), 0) != 0)
                        cout << "Error getting the IP address and port of a Player." << endl << endl;
                    else
                        cout << "[" << host << ":" << service << "] : " << buffer;
                }
                // Fork a new process to handle the request
                pid_t pidTCP = fork();
                if (pidTCP == 0) { // Child process
                    string req = buffer;
                    handle_request_tcp(req, nSock);
                    close(nSock);
                    exit(0);
                }
                close(nSock);
            }
            close(sock);
        }
    }
    // Parent process listens to UDP connections
    else {
        while (true) {
            struct sockaddr_in addrPlayer;
            int sock = create_socket(addrUDP, progName);
            string req = read_from_socket_udp(sock, addrUDP, &addrPlayer);
            string rep = handle_request_udp(req);
            write_to_socket_udp(sock, addrPlayer, rep);
            close(sock);
        }
    }
    return 0;
}

// ================================== UDP Auxiliary Functions ======================================

string read_from_socket_udp(int sock, addrinfo *addr, sockaddr_in* addrPlayer) {
    socklen_t addrLen = sizeof(*addrPlayer);
    char buffer[1024];
    ssize_t n;

    // UDP connection request
    if (addr->ai_socktype == SOCK_DGRAM) {
        if (bind(sock, addr->ai_addr, addr->ai_addrlen) < 0)
            throw runtime_error("Error binding UDP socket");
        // check number of bytes read, end buffer at that location
        n = recvfrom(sock, buffer, 1024, 0, (struct sockaddr*)addrPlayer, &addrLen);
        if (n <= 0)
            throw runtime_error("Error reading from UDP socket");
        buffer[n] = '\0';
        // Print the IP address and Port of the Player, if in verbose mode
        if (verbose) {
            char host[NI_MAXHOST], service[NI_MAXSERV];
            if (getnameinfo((struct sockaddr*)addrPlayer, addrLen, host, sizeof(host), service, sizeof(service), 0) != 0)
                cout << "Error getting the IP address and port of a Player." << endl << endl;
            else
                cout << "[" << host << ":" << service << "] : " << buffer;
        }
    }
    else    // Exception
        throw runtime_error("Invalid address type.");
    return buffer;
}

void write_to_socket_udp(int sock, sockaddr_in addrPlayer, string rep) {
    socklen_t addrLen = sizeof(addrPlayer);
    ssize_t n;

    // UDP connection reply
    n = sendto(sock, rep.c_str(), rep.length(), 0, (struct sockaddr *)&addrPlayer, addrLen);
    if ((int)n == -1 || (int)n != (int)rep.length())
        throw runtime_error("Error sending UDP reply message.");
}

string handle_request_udp(string req) {
    string rep, comm, PLID;
    stringstream ss(req);
    ss >> comm;
    // Start
    if (comm == "SNG") {
        ss >> PLID;
        rep = handle_request_start(PLID);
    }
    // Play
    else if (comm == "PLG") {
        string letter, trial;
        ss >> PLID >> letter >> trial;
        rep = handle_request_play(PLID, letter, trial);
    }
    // Guess
    else if (comm == "PWG") {
        string word, trial;
        ss >> PLID >> word >> trial;
        rep = handle_request_guess(PLID, word, trial);
    }
    // Quit/Exit
    else if (comm == "QUT") {
        ss >> PLID;
        rep = handle_request_quit(PLID);
    }
    // Reveal
    else if (comm == "REV") {
        ss >> PLID;
        rep = handle_request_reveal(PLID);
    }
    else    // Exception
        rep = "ERR";
    return rep + "\0";
}

string handle_request_start(string PLID) {
    string rep = "RSG ";
    string gamePath = get_active_game(PLID);

    // "ERR": The PLID or the syntax was invalid
    // TODO: check the syntax: size of whole request message string is 10
    // gotta do the same for every other handle_request
    if (!is_valid_plid(PLID))
        return rep + "ERR\n";
    // "NOK": The Player has an ongoing game: an active file with moves
    if (gamePath != "" && has_moves(gamePath))
        return rep += "NOK\n";
    // "OK": The Player can start a new game
    if (gamePath != "" && !has_moves(gamePath)) {
        ifstream gameFile(gamePath);
        string line, word;
        getline(gameFile, line);
        stringstream ss(line);
        ss >> word;
        int numLetters = word.length();
        int maxErrors = calculate_max_errors(word.length());
        return rep + "OK " + to_string(numLetters) + " " + to_string(maxErrors) + "\n";
    }
    return rep += "OK " + create_game_file(PLID, wordsFile) + "\n";
}

string handle_request_play(string PLID, string letter, string trial) {
    string rep = "RLG ";
    string gamePath = get_active_game(PLID);

    // "ERR": The PLID is invalid, there is no ongoing game or the syntax is incorrect
    if (!is_valid_plid(PLID) || !is_valid_letter(letter) || !is_valid_integer(trial) || gamePath == "")
        return rep + "ERR\n";
    // Open player game file, get the word and the number of letters and errors
    string line, word, pos;
    fstream gameFile(gamePath);
    getline(gameFile, line);
    stringstream(line) >> word;
    if (word.empty())   // Exception
        throw runtime_error("Error reading word from file.");
    int numLetters = word.length();
    int maxErrors = calculate_max_errors(numLetters);
    // Set a set containing all letters in the word
    set<char> letters;
    for (int i = 0; i < numLetters; i++)
        letters.insert(word[i]);
    // Process previous plays to get the number of errors and trials so far in the game
    string code, play;
    int errCount = 0, tr = 0;
    while (getline(gameFile, line)) {
        tr++;
        stringstream(line) >> code >> play;
        if (code == "T") {
            // "DUP": The played letter was sent in a previous trial
            if (play == letter && tr != stoi(trial))
                return rep + "DUP " + to_string(stoi(trial) - 1) + "\n";
            // Remove from set. If the letter is not in the word, increase the error count 
            else if (letters.erase(play[0]) == 0)
                errCount++;
        }
        // On a previous guess attempt, increase the error count
        else if (code == "G")
            errCount++;
    }
    gameFile.close();
    // cout << "trial: " << trial << " | errCount: " << errCount << " | tr: " << tr << endl; // TODO: rm line
    // "INV": The trial number is invalid
    if (tr != stoi(trial) - 1)
        return rep + "INV " + trial + "\n";
    // Write to file
    gameFile.open(gamePath, ios::out | ios::app);
    gameFile << "T " << letter << endl;
    gameFile.close();
 
    // "OK": The letter is in the word
    if (letters.find(letter[0]) != letters.end()) {
        // "WIN": The Player has won the game
        if (letters.size() == 1) {
            move_to_past_games(PLID, "win");
            return rep + "WIN " + trial + "\n";
        }
        return rep += "OK " + trial + " " + get_letter_positions(word, letter[0]) + "\n";
    }
    // "OVR": The Player has lost the game
    if (errCount == maxErrors) {
        move_to_past_games(PLID, "fail");
        return rep + "OVR " + trial + "\n";
    }
    // "NOK": The letter is not in the word
    return rep + "NOK " + trial + "\n";
}

string handle_request_guess(string PLID, string guess, string trial) {
    string rep = "RWG ";
    string gamePath = get_active_game(PLID);

    // "ERR": The PLID is invalid, there is no ongoing game or the syntax is incorrect
    if (!is_valid_plid(PLID) || !is_valid_word(guess) || !is_valid_integer(trial) || gamePath == "")
        return rep + "ERR\n";
    // Open Player game file and get the word and the maximum number of errors
    string line, word;
    fstream gameFile(gamePath);
    getline(gameFile, line);
    stringstream(line) >> word;
    int numLetters = word.length();
    int maxErrors = calculate_max_errors(numLetters);
    // Set a set containing all letters in the word
    set<char> letters;
    for (int i = 0; i < numLetters; i++)
        letters.insert(word[i]);
    // Process previous plays to get the number of errors and trials so far in the game
    string code, play;
    int errCount = 0, tr = 0;
    while (getline(gameFile, line)) {
        tr++;
        stringstream(line) >> code >> play;
        if (code == "G") {
            // "DUP": The guessed word was sent in a previous trial
            if (play == guess && tr != stoi(trial) - 1)
                return rep + "DUP " + to_string(stoi(trial) - 1) + "\n";
            // Increase the error count
            else
                errCount++;
        }
        // Remove from set. If the letter is not in the word, increase the error count
        else if (code == "T" && letters.erase(play[0]) == 0)
            errCount++;
    }
    gameFile.close();
    // cout << "trial: " << trial << " | tr: " << tr << " | errCount: " << errCount << " | maxErrors: " << maxErrors << endl; // TODO: rm line
    // "INV": The trial number is invalid
    if (tr != stoi(trial) - 1)
        return rep + "INV " + trial + "\n";
    // Write guess to file
    gameFile.open(gamePath, ios::out | ios::app);
    gameFile << "G " << guess << endl;
    gameFile.close();
    // "WIN": The word guess was successful
    if (guess == word) {
        move_to_past_games(PLID, "win");
        return rep + "WIN " + trial + "\n";
    }
    // "OVR": The word guess was not successful and the game is over
    if (errCount == maxErrors) {
        move_to_past_games(PLID, "fail");;
        return rep + "OVR " + trial + "\n";
    }
    // "NOK": The word guess was not successful
    return rep + "NOK " + trial + "\n";
}

string handle_request_quit(string PLID) {
    string rep = "RQT ";

    // "ERR": The PLID or the syntax was invalid
    if (!is_valid_plid(PLID))
        return rep + "ERR\n";
    // "NOK": There is no ongoing game for this Player
    if (get_active_game(PLID) == "")
        return rep + "NOK\n";
    // "OK": The game was successfully terminated
    move_to_past_games(PLID, "quit");
    return rep + "OK\n";
}

string handle_request_reveal(string PLID) {
    string rep = "RRV ";
    string gamePath = get_active_game(PLID);

    // "ERR": The PLID is invalid or there is no ongoing game for this Player
    if (!is_valid_plid(PLID) || gamePath == "")
        return rep + "ERR\n";
    // "word": The game was successfully terminated
    string line, word;
    fstream gameFile(gamePath);
    getline(gameFile, line);
    stringstream(line) >> word;
    gameFile.close();
    move_to_past_games(PLID, "quit");
    return rep + word + "\n";
}

// ================================== TCP Auxiliary Functions ======================================

void setup_socket_tcp(int sock, addrinfo *addr) {
    // TCP connection request
    if (addr->ai_socktype == SOCK_STREAM) {
        while (bind(sock, addr->ai_addr, addr->ai_addrlen) < 0)
            throw runtime_error("Error binding TCP socket");
        // Listen for incoming connections
        if (listen(sock, 10) < 0)
            throw runtime_error("Error listening on TCP socket");
    }
    else    // Exception
        throw runtime_error("Invalid address type.");
}
 
void handle_request_tcp(string req, int sock) {
    string comm, PLID;
    stringstream ss(req);
    ss >> comm;
    // Scoreboard
    if (comm == "GSB")
        handle_request_scoreboard(sock);
    // Hint
    else if (comm == "GHL") {
        ss >> PLID;
        handle_request_hint(PLID, sock);
    }
    // State
    else if (comm == "STA") {
        ss >> PLID;
        handle_request_state(PLID, sock);
    }
    else {  // Exception
        if (send(sock, "ERR\n", 4, 0) < 0)
            throw runtime_error("Error sending TCP reply to unknown command.");
    }
}

void handle_request_scoreboard(int sock) {
    string rep = "RSB ";
    struct scorelist *scoreBoard = get_top_scores();

    // "EMPTY" : There are no finished games yet
    if (scoreBoard == NULL) {
        rep += "EMPTY\n";
        if (send(sock, rep.c_str(), rep.length(), 0) < 0)
            throw runtime_error("Error sending TCP reply to scoreboard request.");
    }
    // "OK": There are finished games and the scoreboard is sent
    else {
        rep += "OK ";
        if (send(sock, rep.c_str(), rep.length(), 0) < 0)
            throw runtime_error("Error sending TCP reply to scoreboard request.");
        string filePath = create_scoreboard_file(scoreBoard);
        send_file(sock, filePath);
        // Delete the temporary file
        if (remove(filePath.c_str()) != 0)
            throw runtime_error("Error deleting temporary scoreboard file.");
    }
}

void handle_request_hint(string PLID, int sock) {
    string rep = "RHL ";
    string filePath = get_hint_file(PLID);

    // "NOK": There is no hint file, or some other problem
    if (filePath == "") {
        rep += "NOK\n";
        if (send(sock, rep.c_str(), rep.length(), 0) < 0)
            throw runtime_error("Error sending TCP reply to hint request.");
    }
    // "OK": The hint file is available and it is sent
    else {
        rep += "OK ";
        if (send(sock, rep.c_str(), rep.length(), 0) < 0)
            throw runtime_error("Error sending TCP reply to hint request.");
        send_file(sock, filePath);
        // Delete the temporary file
        if (remove(filePath.c_str()) != 0)
            throw runtime_error("Error deleting temporary hint file.");
    }
}

void handle_request_state(string PLID, int sock) {
    string rep = "RST ";
    string gamePath = get_active_game(PLID);

    // "NOK": There are no games (active or finished) for this Player, or some other problem
    if (gamePath == "" && get_last_game(PLID) == "") {
        rep += "NOK\n";
        if (send(sock, rep.c_str(), rep.length(), 0) < 0)
            throw runtime_error("Error sending TCP reply to state request.");
    }
    else {
        // "ACT": There is an active game / "FIN": There are only finished games for this Player
        rep += (gamePath != "" ? "ACT " : "FIN ");
        if (send(sock, rep.c_str(), rep.length(), 0) < 0)
            throw runtime_error("Error sending TCP reply to state request.");
        string filePath = create_state_file(PLID);
        send_file(sock, filePath);
        // Delete the temporary file
        if (remove(filePath.c_str()) != 0)
            throw runtime_error("Error deleting temporary state file.");
    }
}

void send_file(int sock, string filePath) {
    // Extract the substring after the last slash
    size_t lastSlashPos = filePath.find_last_of('/');
    string fileName = filePath.substr(lastSlashPos + 1);
    string fileSize = to_string(get_file_size(filePath));
    size_t fileNameLen = fileName.length(), fileSizeLen = fileSize.length();
    fileName[fileNameLen] = '\0';
    fileSize[fileSizeLen] = '\0';
    // Send the file name and size
    if (send(sock, fileName.c_str(), fileNameLen + 1, 0) < 0)
        throw runtime_error("Error sending file name.");
    if (send(sock, fileSize.c_str(), fileSizeLen + 1, 0) < 0)
        throw runtime_error("Error sending file size.");
        
    // Send the file contents
    FILE *fd;
    ssize_t n, bytesRead, bytesSent;
    char buffer[1024];
    // Open the file with read permissions
    if ((fd = fopen(filePath.c_str(), "r")) == NULL)
        throw runtime_error("Error opening file.");
    // Send data from the file to the TCP socket
    memset(buffer, 0, 1024);
    while ((bytesRead = fread(buffer, 1, 1024, fd)) > 0) {
        bytesSent = 0;
        while (bytesSent < bytesRead) {
            if ((n = send(sock, buffer + bytesSent, bytesRead - bytesSent, 0)) < 0)
                throw runtime_error("Error sending file.");
            bytesSent += n;
        }
        memset(buffer, 0, 1024);
    }
    // Close the file
    fclose(fd);
}

// ==================================== Auxiliary Functions ========================================

void handle_signal_gs(int sig) {
    // Close the words file
    if (wordsFile.is_open())
        wordsFile.close();
    // Free the allocated memory for the addresses
    delete addrUDP;
    delete addrTCP;
    // Abort or segmentation fault
    if (sig == SIGABRT || sig == SIGSEGV) {
        cout << endl << "Core dumped." << endl;
        exit(EXIT_FAILURE);
    }
    // Ctrl + C
    if (sig == SIGINT) {
        cout << endl << "Closing process..." << endl;
        // Close all file descriptors (sockets and files)
        for (int i = 0; i < getdtablesize(); i++)
            close(i);
        // Remove all active games left
        string comm = "rm server/games/GAME_*";
        system(comm.c_str());
        exit(0);
    }
}