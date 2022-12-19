#include "../common.h"

// ======================================== Declarations ===========================================

string progName;                    // Program name
struct addrinfo *addrUDP, *addrTCP; // Addrinfo structs for UDP and TCP connections
bool verbose = false;               // Verbose mode
string words_file_name;             // File that stores the words and their hint file
ifstream words_file;                // Input file stream for the words file

void dummy();

string read_from_socket_udp(int sock, addrinfo *addr, struct sockaddr_in *addrPlayer);
void write_to_socket_udp(int sock, struct sockaddr_in addrPlayer, addrinfo *addr, string rep);

int handle_request_tcp(string req, int nsock);
int handle_request_scoreboard(int nsock);
int handle_request_status(string PLID, int nsock);
int handle_request_hint(string PLID, int nsock);

// Function that calls handlers depending on the request
string handle_request_udp(string req);

// Handle the requests from the Player
string handle_request_start(string PLID);
string handle_request_play(string PLID, string letter, string trial);
string handle_request_guess(string PLID, string guess, string trial);
string handle_request_quit(string PLID);
string handle_request_reveal(string PLID);

// Handle the signals from the terminal
void handle_signal_gs(int sig);

// Get the maximum number of errors allowed for a word of the given size
int calculate_max_errors(int len);

// Check if the game file has got moves
bool has_moves(string fname);

void move_to_past_games(string PLID, string status);
string create_top_score_file(struct scorelist* top_scores);
void create_score_file(string PLID, string gamefile);
string find_letter_positions(string word, char letter);


// ============================================ Filesystem ===============================================

// Struct that saves the top 10 scores
struct scorelist {
    int score[10];
    char *PLID[10];
    char *word[10];
    int numSuccess[10];
    int numTrials[10];
    int numScores;
};

string create_game_file(string PLID);
string get_word();
string find_active_game(string PLID);
string find_last_game(string PLID);
scorelist* find_top_scores();

// ============================================ Main ===============================================

int main(int argc, char *argv[]) {
    // Default Game Server's IP address and Port
    string gsIP = "localhost";
    string gsPort = to_string(58000 + GN);

    // Define signal handler to safely exit the program
    if (signal(SIGINT, handle_signal_gs) == SIG_ERR)
        throw runtime_error("Error defining signal handler");

    // Check for input arguments (words_file [-p gsPort] [-v])
    if (argc < 1)   // Exception
        throw invalid_argument("No words file specified");
    // Open the words file with the specified name
    else {
        words_file_name = argv[1];
        words_file.open(words_file_name);
        if (!words_file.is_open())
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
            socklen_t addrlen = sizeof(addrPlayer);
            while (true) {
                int nsock;
                // Accept connection to the server
                if ((nsock = accept(sock, (struct sockaddr *) &addrPlayer, &addrlen)) == -1)
                    throw runtime_error("Error accepting connection to server.");
                // Receive a message
                bytes_read = read(nsock, buffer, 1024);
                if (bytes_read == -1)
                    throw runtime_error("Error receiving TCP request message.");
                buffer[bytes_read] = '\0';
                pid_t pid_tcp = fork();
                if (pid_tcp == 0) { // Child process
                    string req = buffer;
                    int status = handle_request_tcp(req, nsock);
                    close(nsock);
                    exit(0);
                }
                close(nsock);
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
            write_to_socket_udp(sock, addrPlayer, addrUDP, rep);
            close(sock);
        }
    }
    return 0;
}

// ==================================== Auxiliary Functions ========================================

string read_from_socket_udp(int sock, addrinfo *addr, struct sockaddr_in* addrPlayer) {
    socklen_t addrlen = sizeof(*addrPlayer);
    ssize_t bytes_read;
    char buffer[1024];

    // UDP connection request
    if (addr->ai_socktype == SOCK_DGRAM) {
        if (bind(sock, addr->ai_addr, addr->ai_addrlen) < 0)
            throw runtime_error("Error binding UDP socket");
        // check number of bytes read, end buffer at that location
        bytes_read = recvfrom(sock, buffer, 1024, 0, (struct sockaddr *) addrPlayer, &addrlen);
        if (bytes_read <= 0)
            throw runtime_error("Error reading from UDP socket");
        buffer[bytes_read] = '\0';
        // Print the IP address and Port of the Player, if in verbose mode
        if (verbose) {
            char host[NI_MAXHOST], service[NI_MAXSERV];
            if (getnameinfo((struct sockaddr *) addrPlayer, addrlen, host, sizeof(host), service, sizeof(service), 0) != 0)
                cout << "Error getting the IP address and port of a Player." << endl << endl;
            else
                cout << "[" << host << ":" << service << "] : " << buffer << endl;
        }
    }
    else    // Exception
        throw runtime_error("Invalid address type.");
    printf("%s", buffer);   // TODO: rm line
    return buffer;
}

void setup_socket_tcp(int sock, addrinfo *addr, struct sockaddr_in* addrPlayer) {
    socklen_t addrlen = sizeof(*addrPlayer);
    ssize_t bytes_read;

    // TCP connection request
    if (addr->ai_socktype == SOCK_STREAM) {
        if (bind(sock, addr->ai_addr, addr->ai_addrlen) < 0)
            throw runtime_error("Error binding TCP socket");
        if (listen(sock, 10) < 0)
            throw runtime_error("Error listening on TCP socket");
    }
    else    // Exception
        throw runtime_error("Invalid address type.");
}

void write_to_socket_udp(int sock, struct sockaddr_in addrPlayer, addrinfo *addr, string rep) {
    // TODO: do we need addr?
    // UDP connection reply
    if (addr->ai_socktype == SOCK_DGRAM) {
        socklen_t addrlen = sizeof(addrPlayer);
        ssize_t errnumber = sendto(sock, rep.c_str(), rep.length(), 0, (struct sockaddr *)&addrPlayer, addrlen);
        if (errnumber == -1)
            throw runtime_error("Error sending UDP reply message.");
    }
    else    // Exception
        throw runtime_error("Invalid address type.");
}


// ==================================== TCP Request Handlers ========================================
 
int handle_request_tcp(string req , int sock) {
    // TCP commands may be GSB, GHL or STA
    string comm, PLID;
    stringstream ss(req);
    ss >> comm;
    if (comm == "GSB") {
        return handle_request_scoreboard(sock);
    }
    else if (comm == "GHL") {
        ss >> PLID;
        return handle_request_hint(PLID, sock);
    }
    else if (comm == "STA") {
        ss >> PLID;
        return handle_request_status(PLID, sock);
    }
    else
        throw runtime_error("Invalid TCP request.");
    return 1;
}

int handle_request_scoreboard(int sock) {
    struct scorelist *scoreboard = find_top_scores();
    string scoreboard_file = create_top_score_file(scoreboard);
    int chunk_size = 1024;
    int status = send_file(sock, scoreboard_file, chunk_size);
    return status != -1 ? 0 : -1;
}

string create_top_score_file(struct scorelist *scoreboard) {
    pid_t pid = getpid();
    string filename = "server/temp/TOPSCORES_" + to_string(pid) + ".txt";
    ofstream file(filename);
    file << "-------------------------------- TOP 10 SCORES --------------------------------" << endl << endl;
    file << "    SCORE PLAYER     WORD                             GOOD TRIALS  TOTAL TRIALS" << endl;
    for (int i = 0; i < scoreboard->numScores; ++i) {
        file << std::setw(2) << i + 1 << " - " << std::setw(3) << scoreboard->score[i] << "  "
                << std::setw(6) << scoreboard->PLID[i] << "  " << std::setw(30) << scoreboard->word[i] << "  "
                << std::setw(15) << scoreboard->numSuccess[i] << " " << std::setw(15) << scoreboard->numTrials[i]
                << std::endl;
    }

    file.close();
    
    return filename;
}

int send_file(int sock, string scoreboard_file, int chunk_size) {

}

int send_buffer(int sock, const char* buffer, int buffer_size, int chunk_size) { // maybe not needed
    
}
// ==================================== UDP Request Handlers ========================================

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
    string fname = find_active_game(PLID);

    // "ERR": The PLID or the syntax was invalid
    // TODO: gotta check if size of request message string is 10, so maybe pass the string as argument instead of PLID
    if (!is_valid_plid(PLID))
        return rep + "ERR\n";
    // "NOK": The Player has an ongoing game: an active file with moves
    if (fname != "" && !has_moves(fname)) {
        ifstream file(fname);
        string line, word;
        getline(file, line);
        stringstream ss(line);
        ss >> word;
        int num_errors = calculate_max_errors(word.length());
        return rep += "OK " + to_string(num_errors) + "\n";
    }
    if (fname != "" && has_moves(fname))
        return rep += "NOK\n";
    // "OK": The player can start a new game
    return rep += "OK " + create_game_file(PLID) + "\n";
}

string handle_request_play(string PLID, string letter, string trial) {
    string rep = "RLG ";
    string fname = find_active_game(PLID);

    // "ERR": The PLID is invalid, there is no ongoing game or the syntax is incorrect
    if (!is_valid_plid(PLID) || !is_valid_letter(letter) || !is_valid_integer(trial) || fname == "")
        return rep + "ERR\n";

    // Open player game file, get the word and find the positions of the letter
    fstream file(fname, ios::out | ios::in);
    string line, word, pos;
    getline(file, line);
    stringstream(line) >> word;
    if (word.empty())   // Exception
        throw runtime_error("Error reading word from file.");

    int numLetters = word.length();
    int max_errors = calculate_max_errors(numLetters);

    set<char> letters; // Set containing all letters in word
    for (int i = 0; i < numLetters; i++)
        letters.insert(word[i]);
    pos = find_letter_positions(word, letter[0]);

    // Process previous plays
    string cmd, pl;
    int error_count = 0, tr = 0; // error_count: number of errors in the game so far, tr: number of trials so far
    while (getline(file, line)) {
        tr++;
        stringstream(line) >> cmd >> pl;
        if (cmd == "T") {
            // "DUP": The played letter was sent in a previous trial
            if (pl == letter && tr != stoi(trial) - 1)
                return rep + " DUP " + to_string(stoi(trial)-1);
            else if (letters.erase(pl[0]) == 0) { // If the letter is not in the word, increase the error count 
                error_count++;
            } 
        else if (cmd == "G") // If there was a previous guess attempt, increase the error count
                error_count++;
        }
    }
    file.close();

    cout << "trial: " << trial << "error count: " << error_count << "tr: " << tr << endl; // DEBUG

    // "INV": The trial number is invalid (should be one more than the number of trials so far)
    if (tr != stoi(trial) - 1)
        return rep + " INV " + trial;
    // "WIN": The Player has won the game
    if (letters.size() == 0) {
        move_to_past_games(PLID, "win");
        return rep + " WIN " + trial;
    } 
    // "OK": The letter is in the word
    else if (letters.find(letter[0]) != letters.end()) { // check if letter is in letters
        rep += " OK " + trial + " " + pos;
        // write to file
    }
    // "NOK": The letter is not in the word
    else {
        // "OVR": The Player has lost the game
        if (error_count + 1 == max_errors) {
            move_to_past_games(PLID, "fail");
            return rep + " OVR " + trial;
        }
        rep += " NOK " + trial;
    }
    // Write to file
    file.open(fname, ios::out | ios::app);
    file << "T " << letter << endl;
    file.close();
    return rep;
    
}

string handle_request_guess(string PLID, string guess, string trial) {
    string rep = "RWG ";
    string fname = find_active_game(PLID);

    // "ERR": The PLID is invalid, there is no ongoing game or the syntax is incorrect
    if (!is_valid_plid(PLID) || !is_valid_word(guess) || !is_valid_integer(trial) || fname == "")
        return rep += "ERR\n";
    // Open Player game file and get the word and the maximum number of errors
    string line, word;
    fstream file;
    file.open(fname);
    getline(file, line);
    stringstream(line) >> word;
    int max_errors = calculate_max_errors(word.length()), numLetters = word.length();

    set<char> letters; // Set containing all letters in word
    for (int i = 0; i < numLetters; i++)
        letters.insert(word[i]);

    // Get the number of trials and check if the Player has already guessed this word
    string cd, pl;
    int tr = 0, error_count = 0;

    while (getline(file, line)) {
        cout << "line: " << line << endl; // DEBUG
        tr++;
        stringstream(line) >> cd >> pl;
        // "DUP": The guessed word was sent in a previous trial
        if (cd == "G") {
            if (pl != guess)
                error_count++;
            else if (pl == guess && tr != stoi(trial)-1) // DUP is returned if the word was guessed, but not in the last trial
                return rep + "DUP " + to_string(stoi(trial)-1) + "\n";
            else
                return rep + "NOK " + to_string(stoi(trial)-1) + "\n";
        }
        else if (cd == "T" && letters.erase(pl[0]) == 0) // if the letter is not in the word, increase the error count 
            error_count++;
    }
    file.close();

    cout << "debug_guess trial: " << trial << " / tr: " << tr << " error_count: " << error_count << " max_errors: " << max_errors << endl; // DEBUG

    // "INV": The trial number is invalid
    if (tr != stoi(trial) - 1) {
        cout << "trial: " << trial << " / tr: " << tr << " error_count: " << error_count << endl;    // DEBUG
        return rep + "INV " + trial + "\n";
    }
    // "WIN": The word guess was successful
    else if (guess == word) {
        // Write guess to file
        file.open(fname, ios::out | ios::app);
        file << "G " << guess << endl;
        file.close();
        move_to_past_games(PLID, "win");
        rep += "WIN " + trial + "\n";
    }
    // "NOK": The word guess was not successful
    else {
        // "OVR": The word guess was not successful and the game is over
        if (error_count == max_errors) {
            file.open(fname, ios::out | ios::app);
            // Write guess to file
            file << "G " << guess << endl;
            file.close();
            move_to_past_games(PLID, "fail");;
            return rep + "OVR " + trial + "\n";
        }
        rep += "NOK " + trial + "\n";
    }
    // Write guess to file
    file.open(fname, ios::out | ios::app);
    file << "G " << guess << endl;
    file.close();
    return rep;
}

string handle_request_quit(string PLID) {
    string rep = "RQT ";

    // "ERR": The PLID or the syntax was invalid
    if (!is_valid_plid(PLID))   // TODO: how do we know abt the syntax? we need the whole request
        return rep += "ERR\n";
    // "NOK": There is no ongoing game for this Player
    if (find_active_game(PLID) == "")
        return rep += "NOK\n";
    // "OK": The game was successfully terminated
    move_to_past_games(PLID, "quit");
    return rep += "OK\n";
}

string handle_request_reveal(string PLID) {
    string rep = "RRV ";
    string fname = find_active_game(PLID);

    // "ERR": The PLID is invalid or there is no ongoing game for this Player
    if (!is_valid_plid(PLID) || fname == "")
        return rep += "ERR\n";
    // "word": The game was successfully terminated
    fstream file;
    string line, word;
    file.open(fname);
    getline(file, line);
    stringstream(line) >> word;
    file.close();
    move_to_past_games(PLID, "quit");
    return rep + word + "\n";
}

// ============================================ Filesystem ===============================================

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

bool has_moves(string fname) {
    ifstream file(fname);
    string line;
    // If we are able to read a line, check if there is another line
    if (getline(file, line))
        return getline(file, line);
    return false;
}

void move_to_past_games(string PLID, string status) {
    string fname = find_active_game(PLID), date;
    
    if (fname == "")
        throw "No active game found for this PLID";

    create_score_file(PLID, fname);

    map<string, string> status_codes = { {"win", "W"}, {"fail", "F"}, {"quit", "Q"} };
    string mkdir_command = "mkdir -p " + PLID, mv_command = "mv ";
    
    time_t t = time(0);
    tm* now = localtime(&t);
    date = to_string(now->tm_year + 1900) + to_string(now->tm_mon + 1) + 
    to_string(now->tm_mday) + "_" + to_string(now->tm_hour) + to_string(now->tm_min) + to_string(now->tm_sec);
    
    mkdir_command += " server/games/" + PLID;
    mv_command += fname + " server/games/" + PLID + "/" + date + "_" + status_codes[status] + ".txt";
    
    system(mkdir_command.c_str());
    system(mv_command.c_str());
    return;
}

void create_score_file(string PLID, string gamefile) {
    fstream gfile(gamefile);
    // get word from gamefile
    string word, line;
    getline(gfile, line);
    stringstream(line) >> word;

    set<char> letters;
    for (char c : word)
        letters.insert(c);

    int score = 0, n_succ, n_trials;
    string cmd, pl;
    // compute score
    while (getline(gfile, line)) {
        stringstream ss(line);
        n_trials++;
        if (cmd == "G") {
            if (pl == word)
                n_succ++;
        else if (cmd == "T")
            if (letters.find(pl[0]) != letters.end())
                n_succ++;
        }
    }

    // file name should be score_PLID_DDMMYYYY_HHMMSS.txt
    score = n_succ * 100 / n_trials;
    time_t t = time(0);
    tm* now = localtime(&t);
    string date = to_string(now->tm_year + 1900) + to_string(now->tm_mon + 1) + 
    to_string(now->tm_mday) + "_" + to_string(now->tm_hour) + to_string(now->tm_min) + to_string(now->tm_sec);
    string sfile_name = "server/scores/" + to_string(score) + "_" + PLID + "_" + date + ".txt";

    fstream sfile(sfile_name);
    if (!sfile.good()) {
        throw "Could not create score file";
    }
    sfile << to_string(score) << " " << PLID << " " << word << " " << to_string(n_succ) << " " << to_string(n_trials) << endl;
}

string create_game_file(string PLID) {
    string info, word, hint, fname = "server/games/GAME_" + PLID + ".txt";
    ofstream file(fname, ios::in | ios::out | ios::trunc);
    int numLetters, maxErrors;
    // TODO: add exception (could not open file)

    // Choose word and write it and its hint file to the game file
    // file.open(fname, ios::out | ios::in | ios::trunc);

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
    if (words_file.good()) {
        getline(words_file, line);
    } else {
        words_file.close();
        words_file.open(words_file_name);
        getline(words_file, line);
    }
    /*
    vector<string> lines;
    int numLines = 0;
    srand(time(NULL));

    // Open file containing words
    //TODO: check if file exists
    ifstream input(words_file);
    while (getline(input, line)) {
        ++numLines;
        lines.push_back(line);
    }
    int random = rand() % numLines;
    */
    stringstream(line) >> word >> hint;
    return word + " " + hint;
}

string find_active_game(string PLID) {
    string fname = "server/games/GAME_" + PLID + ".txt";
    ifstream file(fname);
    if (!file.good())
        return "";
    else
        return fname;
}

string find_last_game(string PLID) {
    bool found;
    struct dirent **filelist;
    string fname, dname = "server/games/" + PLID + "/";
    int numEntries = scandir(dname.c_str(), &filelist, 0, alphasort);
    if (numEntries <= 0)
        return "";
    else {
        while (numEntries--) {
            if (filelist[numEntries]->d_name[0] != '.') {
                fname = dname + filelist[numEntries]->d_name;
                found = true;
            }
            delete filelist[numEntries];
            if (found)
                break;
        }
        delete filelist;
    }
    return fname;
}

scorelist* find_top_scores() {
    struct dirent **filelist;
    struct scorelist *list = new scorelist;
    string fname, dname = "scores/";
    int i = 0, numEntries = scandir(dname.c_str(), &filelist, 0, alphasort);
    FILE *fp;
    if (numEntries < 0)
        return NULL;
    else {
        while (numEntries--) {
            if (filelist[numEntries]->d_name[0] != '.') {
                fname = dname + filelist[numEntries]->d_name;
                fp = fopen(fname.c_str(), "r");
                if (fp != NULL) {
                    fscanf(fp, "%d %s %s %d %d", &list->score[i], list->PLID[i], list->word[i], &list->numSuccess[i], &list->numTrials[i]);
                    fclose(fp);
                    ++i;
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

string find_letter_positions(string word, char letter) {
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

void handle_signal_gs(int sig) {
    // Ctrl + C
    if (sig == SIGINT) {
        // Close the words file
        if (words_file.is_open())
            words_file.close();
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
    // Do nothing
}