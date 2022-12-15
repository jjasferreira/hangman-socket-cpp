#include "../common.h"

// ======================================== Declarations ===========================================

struct addrinfo *addrUDP, *addrTCP; // addrinfo structs for UDP and TCP connections
bool verbose = false;
string words_file;

void register_player(int PLID, int udp_sock);

string read_from_socket(int sock, addrinfo *addr);
void write_to_socket(int sock, string rep, string prot = "udp");

string handle_request(string req);
// Handle the requests from the player
string handle_request_start(string PLID);
string handle_request_play(string PLID, string letter, string trial);
string handle_request_guess(string PLID, string guess, string trial);
string handle_request_scoreboard(string PLID);
string handle_request_hint(string PLID);
string handle_request_state(string PLID);
string handle_request_quit(string PLID);
string handle_request_reveal(string PLID);

string find_letter_positions(string word, char letter);

// ============================================ Filesystem ===============================================

// Struct saves the top 10 scores
struct scorelist {
    int score[10];
    char *PLID[10];
    char *word[10];
    int numSuccess[10];
    int numTrials[10];
    int numScores;
};

string create_game_file(string PLID);
string get_random_word(string words_file);
string find_active_game(string PLID);
string find_last_game(string PLID);
scorelist* find_top_scores();

// ============================================ Main ===============================================

int main(int argc, char *argv[]) {

    // Default Game Server's IP address and Port
    string gsIP = "localhost";
    string gsPort = to_string(58000 + GN);

    // Determine the words file and overwrite the Port (words_file [-p gsPort] [-v])
    if (argc < 1) // Exception
        throw invalid_argument("No words file specified");
    else
        words_file = argv[1];
    for (int i = 2; i < argc; ++i) {
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
    // Create child process to listen to TCP connections
    if (pid == 0) {
        while (true) {
            /*
            int sock = create_socket();
            string req = read_from_socket(sock, addrTCP);
            handle_request(sock, req);
            close(sock);
            */
        }
    }
    // Parent process listens to UDP connections
    else {
        while (true) {
            int sock = create_socket();
            string req = read_from_socket(sock, addrUDP);
            string rep = handle_request(sock, req);
            write_to_socket(sock, rep);
            close(sock);
        }
        // TODO fork wait / close
    }
    return 0;
}

// ==================================== Auxiliary Functions ========================================

string read_from_socket(int sock, addrinfo *addr) {
    struct sockaddr_in addrPlayer;
    socklen_t addrlen = sizeof(addrPlayer);
    char buffer[1024];

    // TCP connection request
    if (addr->ai_socktype == SOCK_STREAM) {
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

    // UDP connection request
    if (addr->ai_socktype == SOCK_DGRAM) {
        if (bind(sock, addr->ai_addr, addr->ai_addrlen) < 0)
            throw runtime_error("Error binding UDP socket");
        if (recvfrom(sock, buffer, 1024, 0, (struct sockaddr *) &addrPlayer, &addrlen) < 0)
            throw runtime_error("Error reading from UDP socket");
        // Print the IP address and port of the player
        if (verbose) {
            char host[NI_MAXHOST], service[NI_MAXSERV];
            if (getnameinfo((struct sockaddr *) &addrPlayer, addrlen, host, sizeof(host), service, sizeof(service), 0) != 0)
                cout << "Error getting the IP address and port of a Player." << endl << endl;
            else
                cout << "[" << host << ":" << service << "] : " << buffer << endl << endl;
        }
    }
    return buffer;
}

void write_to_socket(int sock, string rep, string prot) {
    // UDP
    if (prot == "udp") {
        if (sendto(sock, rep.c_str(), rep.length(), 0, addrUDP->ai_addr, addrUDP->ai_addrlen) == -1)
            throw runtime_error("Error sending UDP reply message.");
    }
    // TCP
    if (prot == "tcp") {
        if (write(sock, rep.c_str(), rep.length()) == -1)
            throw runtime_error("Error sending TCP reply message.");
    }
}

string handle_request(string req) {
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
        // rep = handle_request_quit(PLID);
    }
    // Reveal
    else if (comm == "REV") {
        ss >> PLID;
        // rep = handle_request_reveal(PLID);
    }
    return rep;
}

string handle_request_start(string PLID) {
    string rep = "RSG";
    // "NOK": The player has an ongoing game
    if (find_active_game(PLID) != "")
        rep += " NOK"; // TODO: If file exists but has no moves, then rep = OK
    // "OK": The player can start a new game
    else
        rep += " OK " + create_game_file(PLID);
    return rep;
}

string handle_request_play(string PLID, string letter, string trial) {
    string rep = "RLG";
    string fname = find_active_game(PLID);
    set<char> letters;

    // "ERR": The PLID is invalid, there is no ongoing game or the syntax is incorrect
    if (!is_valid_plid(PLID) || !is_valid_letter(letter) || !is_valid_integer(trial) || fname == "")
        return rep + " ERR";

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
            letters.erase(cd[0]);
            // "DUP": The played letter was sent in a previous trial
            if (pl == letter && tr != stoi(trial))
                return rep + " DUP ";
        }
    }
    if (tr != stoi(trial) - 1) {
        return rep + " INV " + trial;
    }
    if (letters.size() == 0) {
        return rep + " WIN " + trial;
    }
    else if (tr == maxErrors) {
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
    string rep = "RWG";
    string fname = find_active_game(PLID);

    // "ERR": The PLID is invalid, there is no ongoing game or the syntax is incorrect
    if (!is_valid_plid(PLID) || !is_valid_word(guess) || !is_valid_integer(trial) || fname == "")
        return rep + " ERR ";

    // Open player game file, get the word and find the positions of the letter
    fstream file;
    string line, word, pos;
    int tr = 0;
    file.open(fname);
    getline(file, line);
    stringstream(line) >> word;

    int maxErrors = calculate_max_errors(word.length());

    while (getline(file, line)) {
        tr++;
    }
    
    if (tr != stoi(trial) - 1) {
        return rep + " INV " + trial;
    }
    if (guess == word) {
            rep += " WIN " + trial;
    } else if (tr == maxErrors) {
        return rep + " OVR " + trial;
    } else {
        return rep + " NOK " + trial;
    }
    // write to file
    file << "G " << guess;
    file.close();
    return rep;
}

// TODO place somewhere nicer
string find_letter_positions(string word, char letter) {
    string pos;
    int len = word.length();
    for (int i = 0; i < len; i++) {
        if (word[i] == letter)
            pos += to_string(i + 1) + " ";
    }
    return pos;
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

string create_game_file(string PLID) {
    ofstream file;
    string info, word, hint, fname = "games/GAME_" + PLID + ".txt";
    int numLetters, maxErrors;
    // TODO: is this working or should we use fopen, fwrite?
    // TODO: add exception (could not open file)

    // Choose word and write it and its hint file to the game file
    info = get_random_word(words_file);
    file.open(fname);
    file << info;
    file.close();

    // Set the number of maximum errors for the selected word
    stringstream(info) >> word >> hint;
    numLetters = word.length();
    maxErrors = calculate_max_errors(numLetters);
    return to_string(numLetters) + " " + to_string(maxErrors);
}

string get_random_word(string words_file) {
    string line, word, hint;
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
    stringstream(lines[random]) >> word >> hint;
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