#include "../common.h"

// ======================================== Declarations ===========================================

string progName;                    // Program name
struct addrinfo *addrUDP, *addrTCP; // Addrinfo structs for UDP and TCP connections
bool verbose = false;               // Verbose mode
string wordsFileName;               // File that stores the words and their hint file
ifstream wordsFile;                 // Input file stream for the words file

// Struct that saves the top 10 scores
struct scorelist {
    int score[10];
    string PLID[10];
    string word[10];
    int numSuccess[10];
    int numTrials[10];
    int numScores;
};

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
void setup_socket_tcp(int sock, addrinfo *addr, struct sockaddr_in* addrPlayer);
// Function that calls handlers depending on the TCP request
void handle_request_tcp(string req, int nSock);
// Handle the TCP requests from the Player and send the reply
void handle_request_scoreboard(int nSock);
void handle_request_state(string PLID, int nSock);
void handle_request_hint(string PLID, int nSock);
// Send a file to the Player through the TCP socket
void send_file(int sock, string filePath);
// Get the size of a file on the given path
long get_file_size(string filePath);

// Handle the signals from the terminal
void handle_signal_gs(int sig);

// Get the maximum number of errors allowed for a word of the given size
int calculate_max_errors(int len);

// Check if the game file has got moves
bool has_moves(string filePath);

void move_to_past_games(string PLID, string status);
string create_scoreboard_file(struct scorelist* top_scores);
void create_score_file(string PLID, string gamefile);
string create_state_file(string PLID);
string get_hint_file(string PLID);
string get_letter_positions(string word, char letter);

string create_game_file(string PLID);
string get_word();
string get_active_game(string PLID);
string get_last_game(string PLID);
scorelist* get_top_scores();

bool debug = true;
void dummy();   // TODO: rm

// ============================================ Main ===============================================

int main(int argc, char *argv[]) {
    // Default Game Server's IP address and Port
    string gsIP = "localhost";
    string gsPort = to_string(58000 + GN);

    // Define signal handler to safely exit the program
    if (signal(SIGINT, handle_signal_gs) == SIG_ERR)
        throw runtime_error("Error defining signal handler");

    // Check for input arguments (wordsFile [-p gsPort] [-v])
    if (argc < 1)   // Exception
        throw invalid_argument("No words file specified");
    // Open the words file with the specified name
    else {
        wordsFileName = argv[1];
        wordsFile.open(wordsFileName);
        if (!wordsFile.is_open())
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
            struct sockaddr_in addrPlayer;
            int sock = create_socket(addrTCP, progName);
            setup_socket_tcp(sock, addrTCP, &addrPlayer);

            char buffer[1024];
            ssize_t bytes_read;
            socklen_t addrLen = sizeof(addrPlayer);
            while (true) {
                int nSock;
                // Accept connection to the server
                if ((nSock = accept(sock, (struct sockaddr *) &addrPlayer, &addrLen)) == -1)
                    throw runtime_error("Error accepting connection to server.");
                // Receive a message
                bytes_read = read(nSock, buffer, 1024);
                if (bytes_read == -1)
                    throw runtime_error("Error receiving TCP request message.");
                buffer[bytes_read] = '\0';
                if (debug) cout << "Received TCP request: " << buffer << endl;
                pid_t pid_tcp = fork();
                if (pid_tcp == 0) { // Child process
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
        n = recvfrom(sock, buffer, 1024, 0, (struct sockaddr *) addrPlayer, &addrLen);
        if (n <= 0)
            throw runtime_error("Error reading from UDP socket");
        buffer[n] = '\0';
        // Print the IP address and Port of the Player, if in verbose mode
        if (verbose) {
            char host[NI_MAXHOST], service[NI_MAXSERV];
            if (getnameinfo((struct sockaddr *) addrPlayer, addrLen, host, sizeof(host), service, sizeof(service), 0) != 0)
                cout << "Error getting the IP address and port of a Player." << endl << endl;
            else
                cout << "[" << host << ":" << service << "] : " << buffer << endl;
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
    string filePath = get_active_game(PLID);

    // "ERR": The PLID or the syntax was invalid
    // TODO: check the syntax: size of whole request message string is 10
    // gotta do the same for every other handle_request
    if (!is_valid_plid(PLID))
        return rep + "ERR\n";
    // "NOK": The Player has an ongoing game: an active file with moves
    if (filePath != "" && has_moves(filePath))
        return rep += "NOK\n";
    // "OK": The Player can start a new game
    if (filePath != "" && !has_moves(filePath)) {
        ifstream file(filePath);
        string line, word;
        getline(file, line);
        stringstream ss(line);
        ss >> word;
        int numLetters = word.length();
        int maxErrors = calculate_max_errors(word.length());
        return rep + "OK " + to_string(numLetters) + " " + to_string(maxErrors) + "\n";
    }
    return rep += "OK " + create_game_file(PLID) + "\n";
}

string handle_request_play(string PLID, string letter, string trial) {
    string rep = "RLG ";
    string filePath = get_active_game(PLID);

    // "ERR": The PLID is invalid, there is no ongoing game or the syntax is incorrect
    if (!is_valid_plid(PLID) || !is_valid_letter(letter) || !is_valid_integer(trial) || filePath == "")
        return rep + "ERR\n";
    // Open player game file, get the word and the number of letters and errors
    string line, word, pos;
    fstream file(filePath);
    getline(file, line);
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
    while (getline(file, line)) {
        tr++;
        stringstream(line) >> code >> play;
        if (code == "T") {
            // "DUP": The played letter was sent in a previous trial
            if (play == letter && tr != stoi(trial) - 1)
                return rep + "DUP " + to_string(stoi(trial) - 1) + "\n";
            // Remove from set. If the letter is not in the word, increase the error count 
            else if (letters.erase(play[0]) == 0)
                errCount++;
        }
        // On a previous guess attempt, increase the error count
        else if (code == "G")
            errCount++;
    }
    file.close();
    cout << "trial: " << trial << " | errCount: " << errCount << " | tr: " << tr << endl; // TODO: rm line
    // "INV": The trial number is invalid
    if (tr != stoi(trial) - 1)
        return rep + "INV " + trial + "\n";
    // Write to file
    file.open(filePath, ios::out | ios::app);
    file << "T " << letter << endl;
    file.close();
 
    // "OK": The letter is in the word
    if (letters.find(letter[0]) != letters.end()) {
        // "WIN": The Player has won the game
        if (letters.size() == 1) {
            move_to_past_games(PLID, "win");
            return rep + "WIN " + trial + "\n";
        }
        return rep += "OK " + trial + " " + get_letter_positions(word, letter[0]);
    }
    // "OVR": The Player has lost the game
    if (errCount + 1 == maxErrors) {
        move_to_past_games(PLID, "fail");
        return rep + "OVR " + trial + "\n";
    }
    // "NOK": The letter is not in the word
    return rep + "NOK " + trial + "\n";
}

string handle_request_guess(string PLID, string guess, string trial) {
    string rep = "RWG ";
    string filePath = get_active_game(PLID);

    // "ERR": The PLID is invalid, there is no ongoing game or the syntax is incorrect
    if (!is_valid_plid(PLID) || !is_valid_word(guess) || !is_valid_integer(trial) || filePath == "")
        return rep + "ERR\n";
    // Open Player game file and get the word and the maximum number of errors
    string line, word;
    fstream file(filePath);
    getline(file, line);
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
    while (getline(file, line)) {
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
    file.close();
    cout << "trial: " << trial << " | tr: " << tr << " | errCount: " << errCount << " | maxErrors: " << maxErrors << endl; // TODO: rm line
    // "INV": The trial number is invalid
    if (tr != stoi(trial) - 1)
        return rep + "INV " + trial + "\n";
    // Write guess to file
    file.open(filePath, ios::out | ios::app);
    file << "G " << guess << endl;
    file.close();
    // "WIN": The word guess was successful
    if (guess == word) {
        move_to_past_games(PLID, "win");
        return rep + "WIN " + trial + "\n";
    }
    // "OVR": The word guess was not successful and the game is over
    if (errCount + 1 == maxErrors) {
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
    string filePath = get_active_game(PLID);

    // "ERR": The PLID is invalid or there is no ongoing game for this Player
    if (!is_valid_plid(PLID) || filePath == "")
        return rep + "ERR\n";
    // "word": The game was successfully terminated
    fstream file;
    string line, word;
    file.open(filePath);
    getline(file, line);
    stringstream(line) >> word;
    file.close();
    move_to_past_games(PLID, "quit");
    return rep + word + "\n";
}

// ================================== TCP Auxiliary Functions ======================================

void setup_socket_tcp(int sock, addrinfo *addr, struct sockaddr_in* addrPlayer) {
    // socklen_t addrLen = sizeof(*addrPlayer); TODO these aren't being used
    // ssize_t n;

    // TCP connection request
    if (addr->ai_socktype == SOCK_STREAM) {
        if (bind(sock, addr->ai_addr, addr->ai_addrlen) < 0)
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

string create_scoreboard_file(struct scorelist *sb) {
    // Create a scoreboard file with a unique name
    pid_t pid = getpid();
    string filePath = "server/temp/TOPSCORES_" + to_string(pid) + ".txt";
    ofstream file(filePath, ios::out | ios::trunc);
    // Write the scoreboard to the opened file
    file << "-------------------------------- TOP 10 SCORES --------------------------------" << endl << endl;
    file << "    SCORE PLAYER     WORD                             GOOD TRIALS  TOTAL TRIALS" << endl << endl;
    for (int i = 0; i < sb->numScores; ++i) {
        file << left << " " << setw(1) << i + 1 << " - " << setw(3) << sb->score[i] << "  "
                << setw(8) << sb->PLID[i] << setw(38) << sb->word[i] << "  "
                << setw(14) << sb->numSuccess[i] << sb->numTrials[i] << endl;
    }
    // Close the file and return its path
    file.close();
    return filePath;
}

string get_hint_file(string PLID) {
    string filePath = get_active_game(PLID);

    // If there is no active game, return an empty string
    if (filePath == "")
        return "";
    // If there is an active game, return the path to the hint file
    ifstream file(filePath);
    string line, word, hint;
    getline(file, line);
    stringstream(line) >> word >> hint;
    file.close();
    return "server/images/" + hint;
}

string create_state_file(string PLID) {
    string statePath, filePath = get_active_game(PLID);
    bool activeGame = true;
    char termState = ' ';
    // If there is no active game, find the last finished game
    if (filePath == "") {
        filePath = get_last_game(PLID);
        activeGame = false;
        // get the termination state of the last game
        size_t lastUndScPos = filePath.find_last_of('_');
        termState = filePath.substr(lastUndScPos + 1)[0];
        if (filePath == "") // Exception
            throw runtime_error("No games (active or finished) found for this Player.");
    }
    fstream file(filePath);
    if (!file.is_open()) // Exception
        throw runtime_error("Error opening game file.");
    string line, word, hint;
    getline(file, line);
    stringstream(line) >> word >> hint;

    // Create a state file with a unique name
    statePath = "server/temp/STATE_" + PLID + ".txt";
    ofstream stateFile(statePath, ios::out | ios::trunc);

    // Write the state to the opened file
    // Write header
    if (activeGame)
        stateFile << "Active game found for player " << PLID << endl;
    else
        stateFile << "Last finalized game for player " << PLID << endl;
    stateFile << "Word: " << word << "; Hint file: " << hint << endl;

    // Compute transaction count
    int transCount = 0;
    while(getline(file, line)) {
        if (line != "")
            ++transCount;
    }
    stateFile << "--- Transactions found: " << transCount << " ---" << endl;
    file.close();
    file.open(filePath);
    getline(file, line); // Skip the first line
    // create set with letters from word
    set<char> letters;
    string wordProgress;
    for (char c : word)
        letters.insert(c);

    // Set for guessed letters
    set<char> guessed;

    string cmd, pl;
    while(getline(file, line)) {
        stringstream(line) >> cmd >> pl;
        if (cmd == "T") {
            if (letters.find(pl[0]) != letters.end()) {
                stateFile << "Letter trial: " << pl << " - TRUE" << endl;
                guessed.insert(pl[0]);
            }
            else {
                stateFile << "Letter trial: " << pl << " - FALSE" << endl;
            }
        } else if (cmd == "G") {
            stateFile << "Word guess: " << pl << endl;
        }
    }
    if (activeGame) {
        // iterate through elements of set in order to build wordProgress indicator
        string wordProgress;
        for (char c : word) {
            if (guessed.find(c) != guessed.end())
                wordProgress += c;
            else
                wordProgress += '_';
        }
        stateFile << "Solved so far: " << wordProgress << endl;
    }
    else {
        map<char, string> termStates = {{'W', "WIN"}, {'F', "FAIL"}, {'Q', "QUIT"}};
        stateFile << "Termination: " << termStates[termState] << endl;
    }
    return statePath;
}

void send_file(int sock, string filePath) {
    // Extract the substring after the last slash
    size_t lastSlashPos = filePath.find_last_of('/');
    string fileName = filePath.substr(lastSlashPos + 1);
    string fileSize = to_string(get_file_size(filePath));
    size_t fname_len = fileName.length(), fsize_len = fileSize.length();
    fileName[fname_len] = '\0';
    fileSize[fsize_len] = '\0';
    // Send the file name and size
    if (debug) cout << "Sending file name: " << fileName << endl;
    if (send(sock, fileName.c_str(), fname_len + 1, 0) < 0)
        throw runtime_error("Error sending file name.");
    if (send(sock, fileSize.c_str(), fsize_len + 1, 0) < 0)
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
        if (debug) cout << "Sending " << bytesRead << " bytes: " << buffer << endl;
        bytesSent = 0;
        while (bytesSent < bytesRead) {
            if ((n = send(sock, buffer + bytesSent, bytesRead - bytesSent, 0)) < 0)
                throw runtime_error("Error sending file.");
            bytesSent += n;
        }
        memset(buffer, 0, 1024);
    }
    // Close the file and print the result
    fclose(fd);
}

long get_file_size(string filePath) {
    struct stat statBuf;
    int res = stat(filePath.c_str(), &statBuf);
    return res == 0 ? statBuf.st_size : -1;
}

// ========================================= Filesystem ============================================

bool has_moves(string filePath) {
    ifstream file(filePath);
    string line;
    // If we are able to read a line, check if there is another line
    if (getline(file, line)) {
        getline(file, line);
        return (line.length() > 0);
    }
    return false;
}

void move_to_past_games(string PLID, string status) {
    string filePath = get_active_game(PLID), date;
    
    if (filePath == "")
        throw runtime_error("No active game found for this PLID");

    create_score_file(PLID, filePath);

    map<string, string> status_codes = { {"win", "W"}, {"fail", "F"}, {"quit", "Q"} };
    string mkdir_command = "mkdir -p " + PLID, mv_command = "mv ";
    
    time_t t = time(0);
    tm* now = localtime(&t);
    date = to_string(now->tm_year + 1900) + to_string(now->tm_mon + 1) + 
    to_string(now->tm_mday) + "_" + to_string(now->tm_hour) + to_string(now->tm_min) + to_string(now->tm_sec);
    
    mkdir_command += " server/games/" + PLID;
    mv_command += filePath + " server/games/" + PLID + "/" + date + "_" + status_codes[status] + ".txt";
    
    system(mkdir_command.c_str());
    system(mv_command.c_str());
    return;
}

void create_score_file(string PLID, string gamefile) {
    fstream gfile(gamefile);
    if (!gfile.is_open())
        cout << "Error opening game file" << endl;
    // get word from gamefile
    string word, line;
    getline(gfile, line);
    stringstream(line) >> word;

    set<char> letters;
    for (char c : word)
        letters.insert(c);

    dummy();

    int score = 0, n_succ = 0, n_trials = 0;
    string cmd, pl;
    // compute score
    while (getline(gfile, line)) {
        n_trials++;
        stringstream ss(line);
        cout << line << endl;
        ss >> cmd >> pl;
        if (cmd == "G") {
            if (pl == word)
                n_succ++;
        }
        else if (cmd == "T") {
            if (letters.find(pl[0]) != letters.end())
                n_succ++;
        }
    }

    cout << "n_succ: " << n_succ << " n_trials: " << n_trials << endl;
    // file name should be score_PLID_DDMMYYYY_HHMMSS.txt
    score = n_succ * 100 / n_trials;
    time_t t = time(0);
    tm* now = localtime(&t);
    string date = to_string(now->tm_year + 1900) + to_string(now->tm_mon + 1) + 
    to_string(now->tm_mday) + "_" + to_string(now->tm_hour) + to_string(now->tm_min) + to_string(now->tm_sec);
    string sfile_name = "server/scores/" + to_string(score) + "_" + PLID + "_" + date + ".txt";

    ofstream sfile;
    sfile.open(sfile_name, ios::out | ios::in | ios::trunc);
    cout << sfile_name << endl;
    if (!sfile.good()) {
        throw runtime_error("Could not create score file");
    }
    sfile << to_string(score) << " " << PLID << " " << word << " " << to_string(n_succ) << " " << to_string(n_trials) << endl;
}

string create_game_file(string PLID) {
    string info, word, hint, filePath = "server/games/GAME_" + PLID + ".txt";
    ofstream file(filePath, ios::in | ios::out | ios::trunc);
    int numLetters, maxErrors;
    // TODO: add exception (could not open file)

    // Choose word and write it and its hint file to the game file
    // file.open(filePath, ios::out | ios::in | ios::trunc);

    info = get_word();
    file << info << endl;
    
    file.close();

    // Set the number of maximum errors for the selected word
    stringstream(info) >> word >> hint;
    numLetters = word.length();
    maxErrors = calculate_max_errors(numLetters);
    return to_string(numLetters) + " " + to_string(maxErrors);
}

string get_word() {
    string line, word, hint;
    if (wordsFile.good()) {
        getline(wordsFile, line);
    } else {
        wordsFile.close();
        wordsFile.open(wordsFileName);
        getline(wordsFile, line);
    }
    /*
    vector<string> lines;
    int numLines = 0;
    srand(time(NULL));

    // Open file containing words
    //TODO: check if file exists
    ifstream input(wordsFile);
    while (getline(input, line)) {
        ++numLines;
        lines.push_back(line);
    }
    int random = rand() % numLines;
    */
    stringstream(line) >> word >> hint;
    return word + " " + hint;
}

string get_active_game(string PLID) {
    string filePath = "server/games/GAME_" + PLID + ".txt";
    ifstream file(filePath);
    if (!file.good())
        return "";
    else
        return filePath;
}

string get_last_game(string PLID) {
    struct dirent **filelist;
    string filePath, dir = "server/games/" + PLID + "/";
    bool found;
    // Scan the directory for the last game
    int numEntries = scandir(dir.c_str(), &filelist, 0, alphasort);
    if (numEntries <= 0)
        return "";
    else {
        while (numEntries--) {
            // Check if the file is a regular file
            if (filelist[numEntries]->d_type == DT_REG) {
                filePath = dir + filelist[numEntries]->d_name;
                found = true;
            }
            delete filelist[numEntries];
            if (found)
                break;
        }
        delete filelist;
    }
    return filePath;
}

scorelist* get_top_scores() {
    struct dirent **filelist;
    struct scorelist *list = new scorelist;
    string filePath, dir = "server/scores/";
    // Scan the directory for the top 10 scores
    int i = 0, numEntries = scandir(dir.c_str(), &filelist, 0, alphasort);
    if (numEntries < 0)
        return NULL;
    else {
        while (numEntries--) {
            // Check if the file is a regular file
            if (filelist[numEntries]->d_type == DT_REG) {
                filePath = dir + filelist[numEntries]->d_name;
                ifstream file(filePath);
                if (file.is_open()) {
                    string line;
                    getline(file, line);
                    stringstream(line) >> list->score[i] >> list->PLID[i] >> list->word[i] >> list->numSuccess[i] >> list->numTrials[i];
                    file.close();
                    i++;
                }
            }
            delete filelist[numEntries];
            if (i == 10)
                break;
        }
        delete filelist;
    }
    list->numScores = i;
    return list;
}

// ==================================== Auxiliary Functions ========================================

string get_letter_positions(string word, char letter) {
    string pos;
    int occ = 0;
    int len = word.length();
    for (int i = 0; i < len; i++) {
        if (word[i] == letter) {
            pos += to_string(i + 1) + " ";
            occ++;
        }
    }
    return to_string(occ) + " " + pos;
}

int calculate_max_errors(int len) {
    int res;
    if (len <= 6)
        res = 7;
    else if (len <= 10)
        res = 8;
    else
        res = 9;
    return res;
}

void handle_signal_gs(int sig) {
    // Ctrl + C
    if (sig == SIGINT) {
        // Close the words file
        if (wordsFile.is_open())
            wordsFile.close();
        // Close all file descriptors (sockets and files)
        for (int i = 0; i < getdtablesize(); i++)
            close(i);
        // Free the allocated memory for the addresses
        delete addrUDP;
        delete addrTCP;
        cout << "Closing the server..." << endl;
        exit(0);
    }
}

void dummy() {
    // Do nothing TODO: rm
}