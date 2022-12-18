#include "../common.h"

// ======================================== Declarations ===========================================

struct addrinfo *addrUDP, *addrTCP; // addrinfo structs for UDP and TCP connections
bool verbose = false;
string words_file_name;
ifstream words_file;
string progName;

void register_player(int PLID, int udp_sock);

string read_from_socket(int sock, addrinfo *addr);
void write_to_socket(int sock, addrinfo *addr, string rep);

string handle_request_udp(string req);
// Handle the requests from the player
string handle_request_start(string PLID);
string handle_request_play(string PLID, string letter, string trial);
string handle_request_guess(string PLID, string guess, string trial);
string handle_request_scoreboard(string PLID);
string handle_request_hint(string PLID);
string handle_request_state(string PLID);
string handle_request_quit(string PLID);
string handle_request_reveal(string PLID);

bool has_no_moves(string filename);
void move_to_past_games(string PLID, string status);
string find_letter_positions(string word, char letter);
int calculate_max_errors(int len);

void handle_signal_gs(int sig);

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
            // TODO: handle TCP connections
            /*
            int sock = create_socket(addrTCP, progName);
            string req = read_from_socket(sock, addrTCP);
            handle_request(req);
            close(sock);
            */
        }
    }
    // Parent process listens to UDP connections
    else {
        while (true) {
            int sock = create_socket(addrUDP, progName);
            cout << "debug5: " << endl; // TODO: rm line
            string req = read_from_socket(sock, addrUDP);
            cout << "debug2: " << endl; // TODO: rm line
            string rep = handle_request_udp(req);
            cout << "debug3: " << rep << endl;  // TODO: rm line
            write_to_socket(sock, addrUDP, rep);
            cout << "debug4: " << endl; // TODO: rm line
            close(sock);
        }
    }
    return 0;
}

// ==================================== Auxiliary Functions ========================================

string read_from_socket(int sock, addrinfo *addr) {
    struct sockaddr_in addrPlayer;
    socklen_t addrlen = sizeof(addrPlayer);
    char buffer[1024];

    // UDP connection request
    if (addr->ai_socktype == SOCK_DGRAM) {
        if (bind(sock, addr->ai_addr, addr->ai_addrlen) < 0)
            throw runtime_error("Error binding UDP socket");
        if (recvfrom(sock, buffer, 1024, 0, (struct sockaddr *) &addrPlayer, &addrlen) < 0)
            throw runtime_error("Error reading from UDP socket");
        // Print the IP address and Port of the Player, if in verbose mode
        if (verbose) {
            char host[NI_MAXHOST], service[NI_MAXSERV];
            if (getnameinfo((struct sockaddr *) &addrPlayer, addrlen, host, sizeof(host), service, sizeof(service), 0) != 0)
                cout << "Error getting the IP address and port of a Player." << endl; // TODO why are there two endl's?
            else
                cout << "[" << host << ":" << service << "] : " << buffer << endl;
        }
    }
    // TCP connection request
    else if (addr->ai_socktype == SOCK_STREAM) {
        if (bind(sock, addr->ai_addr, addr->ai_addrlen) < 0)
            throw runtime_error("Error binding TCP socket");
        if (listen(sock, 10) < 0)
            throw runtime_error("Error listening on TCP socket");
        while (true) {
            int nsock;
            // Accept connection to the server
            if ((nsock = accept(sock, (struct sockaddr *) &addrPlayer, &addrlen)) == -1)
                throw runtime_error("Error accepting connection to server.");
            // Receive a message
            if (read(nsock, buffer, 1024) == -1)
                throw runtime_error("Error receiving TCP request message.");
            close(nsock);
            break; //TODO: have to return nsock as well
        }
    }
    else    // Exception
        throw runtime_error("Invalid address type.");
    cout << "debug7: " << buffer << endl;   // TODO: rm line
    return buffer;
}

void write_to_socket(int sock, addrinfo *addr, string rep) {
    // UDP connection reply
    if (addr->ai_socktype == SOCK_DGRAM) {
        cout << "debug6: " << endl; // TODO: rm line
        if (sendto(sock, rep.c_str(), rep.length(), 0, addrUDP->ai_addr, addrUDP->ai_addrlen) == -1)
            throw runtime_error("Error sending UDP reply message.");
    }
    // TCP connection reply
    else if (addr->ai_socktype == SOCK_STREAM) {
        if (write(sock, rep.c_str(), rep.length()) == -1)
            throw runtime_error("Error sending TCP reply message.");
    }
    else    // Exception
        throw runtime_error("Invalid address type.");
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
    return rep;
}

string handle_request_start(string PLID) {
    string rep = "RSG ";
    string fname = find_active_game(PLID);

    // "ERR": The PLID or the syntax was invalid
    // TODO: gotta check if size of request message string is 10, so maybe pass the string as argument instead of PLID
    if (!is_valid_plid(PLID))
        return rep + "ERR\n";
    // "NOK": The Player has an ongoing game: an active file with moves
    if (fname != "" && !has_no_moves(fname))  // TODO: change logic to has_moves_gfile
        return rep += "NOK\n";
    // "OK": The player can start a new game
    return rep += "OK " + create_game_file(PLID) + "\n";
}

string handle_request_play(string PLID, string letter, string trial) {
    string rep = "RLG ";
    string fname = find_active_game(PLID);
    set<char> letters;

    // "ERR": The PLID is invalid, there is no ongoing game or the syntax is incorrect
    if (!is_valid_plid(PLID) || !is_valid_letter(letter) || !is_valid_integer(trial) || fname == "")
        return rep + "ERR\n";

    // Open player game file, get the word and find the positions of the letter
    fstream file;
    string line, word, pos;
    file.open(fname);
    getline(file, line);
    stringstream(line) >> word;
    
    // Set the set of letters
    int numLetters = word.length();
    int maxErrors = calculate_max_errors(numLetters);
    for (int i = 0; i < numLetters; i++)
        letters.insert(word[i]);
    pos = find_letter_positions(word, letter[0]);

    // Check if the Player has already played this letter
    string cd, pl;
    int tr = 0;
    while (getline(file, line)) {
        tr++;
        stringstream(line) >> cd >> pl;
        if (cd == "T") {
            // "DUP": The played letter was sent in a previous trial
            if (pl == letter && tr != stoi(trial))
                return rep + " DUP ";
            letters.erase(pl[0]);
        }
    }
    if (tr != stoi(trial) - 1)
        return rep + " INV " + trial;
    if (letters.size() == 0) {
        move_to_past_games(PLID, "win");
        return rep + " WIN " + trial;
    }
    else if (tr == maxErrors) {
        move_to_past_games(PLID, "fail");
        return rep + " OVR " + trial;
    }
    else {
        rep += " OK " + trial + " " + pos;
        // write to file
        file << "T " << letter;
        file.close();
        return rep;
    }
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
    int maxErrors = calculate_max_errors(word.length());

    // Get the number of trials and check if the Player has already guessed this word
    string cd, pl;
    int tr = 0;
    while (getline(file, line)) {
        tr++;
        stringstream(line) >> cd >> pl;
        // "DUP": The guessed word was sent in a previous trial
        if (cd == "G" && pl == guess && tr != stoi(trial))
            return rep += "DUP " + trial + "\n";
    }
    // "INV": The trial number is invalid
    if (tr != stoi(trial) - 1) {
        cout << "trial: " << trial << "/ tr: " << tr << endl;    // TODO: rm line
        return rep + "INV " + trial + "\n";
    }
    file << "G " << guess << endl;
    file.close();
    // "WIN": The word guess was successful
    if (guess == word) {
        move_to_past_games(PLID, "win");
        return rep + "WIN " + trial + "\n";
    }
    // "OVR": The word guess was not successful and the game is over
    if (guess != word && tr == maxErrors) { // TODO: its not as simple as this
        move_to_past_games(PLID, "fail");;
        return rep + "OVR " + trial + "\n";
    }
    // "NOK": The word guess was not successful
    return rep + "OVR " + trial + "\n";
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

bool has_no_moves(string filename) {
    // Check if the file only has one line
    ifstream file(filename);
    string line;
    return file.good() && std::getline(file, line) && file.eof();
}

void move_to_past_games(string PLID, string status) {
    string fname = find_active_game(PLID), date;
    if (fname == "")
        throw "No active game found for this PLID";

    map<string, string> status_codes = { {"win", "W"}, {"fail", "F"}, {"quit", "Q"} };
    string mkdir_command = "mkdir -p " + PLID, mv_command = "mv ";
    
    time_t t = time(0);
    tm* now = localtime(&t);
    date = to_string(now->tm_year + 1900) + to_string(now->tm_mon + 1) + 
    to_string(now->tm_mday) + to_string(now->tm_hour) + to_string(now->tm_min) + to_string(now->tm_sec);
    
    mv_command += fname + " " + PLID + "/" + date + "_" + status_codes[status] + ".txt";
    
    system(mkdir_command.c_str());
    system(mv_command.c_str());
    return;
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

string create_game_file(string PLID) {
    fstream file;
    string info, word, hint, fname = "games/GAME_" + PLID + ".txt";
    int numLetters, maxErrors;
    // TODO: add exception (could not open file)

    // Choose word and write it and its hint file to the game file
    file.open(fname);

    if (has_no_moves(fname)) // if it exists and has no moves, we get the info from the file
        getline(file, info);
    else {
        info = get_word();
        file << info;
    }
    
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
    string fname = "games/GAME_" + PLID + ".txt";
    ifstream file(fname);
    if (!file.good())
        return "";
    else
        return fname;
}

string find_last_game(string PLID) {
    bool found;
    struct dirent **filelist;
    string fname, dname = "games/" + PLID + "/";
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
    int len = word.length();
    for (int i = 0; i < len; i++) {
        if (word[i] == letter)
            pos += to_string(i + 1) + " ";
    }
    return pos;
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