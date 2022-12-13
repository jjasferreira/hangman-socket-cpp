#include "../common.h"

// ======================================== Declarations ===========================================

struct addrinfo *addrUDP, *addrTCP; // addrinfo structs for UDP and TCP connections
string gsIP = "localhost";
string gsPort = to_string(58000 + GN);
bool verbose = false;
string words_file;

void handle_request_udp(char *buffer, int n, int udp_sock, addrinfo *addr);
void register_player(int PLID, int udp_sock);

// Handle the requests from the player
void handle_request_play(string rep, string arg, addrinfo *addr);
void handle_request_guess(string rep, string arg, addrinfo *addr);
void handle_request_start(string rep, string arg, addrinfo *addr);
void handle_request_scoreboard(string rep, string arg, int sock, addrinfo *addr);
void handle_request_hint(string rep, string arg, int sock, addrinfo *addr);
void handle_request_state(string rep, string arg, int sock, addrinfo *addr);
void handle_request_quit(string rep, string arg, addrinfo *addr);
void handle_request_reveal(string rep, string arg, addrinfo *addr);

// ============================================ Filesystem ===============================================

struct scorelist {
    // saves the top 10 scores
    int score[10];
    char *PLID[10];
    char *word[10];
    // number of successful trials
    int numSuccess[10];
    // number of trials
    int numTrials[10];
    // number of scores
    int numScores;
};

int find_last_game(char *PLID, char *fname);
int find_top_scores(scorelist *list);
string new_game_file(string PLID);

// ============================================ Main ===============================================

int main(int argc, char *argv[]) {

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

        // Create a TCP socket
        int sock = create_socket("tcp");

        if (bind(sock, res->ai_addr, res->ai_addrlen) < 0)
            throw runtime_error("Error binding TCP socket");
        if (listen(sock, 10) < 0)
            throw runtime_error("Error listening on TCP socket");

        while (true) {
            int nsock;
            // Accept connection to the server
            if ((nsock = accept(sock, (struct sockaddr *)&address, &addrlen)) == -1)
                throw runtime_error("Error accepting connection to server.");
            // Receive a message
            if (read(nsock, buffer, 1024) == -1)
                throw runtime_error("Error receiving TCP request message.");
            cout << buffer; // TODO: rm line
            // Send a message
            /* if (write(nsock, buffer, 1024) == -1) // TODO: change size of file
                throw runtime_error("Error sending TCP reply message."); */
            close(nsock);
            break;
            // TODO: Handle TCP requests
        }
        close(sock);

    
    }
    // Parent process listens to UDP connections
    else {
        while (true) {
            int sock = create_socket();
            string req = read_from_socket(sock);
            cout << "Received UDP message: " << req << endl; // TODO: rm line
            handle_request(sock, req);
            close(sock);
        }
    }
    return 0;
}

string read_from_socket(int sock) {
    struct sockaddr_in address;
    socklen_t addrlen;
    char buffer[1024];

    // UDP
    if (bind(sock, addrUDP->ai_addr, addrUDP->ai_addrlen) < 0)
        throw runtime_error("Error binding UDP socket");
    if (recvfrom(sock, buffer, 1024, 0, (struct sockaddr *)&address, &addrlen) < 0)
        throw runtime_error("Error reading from UDP socket");
    return buffer;
}

void write_to_socket(int sock, string rep, addrinfo *addr) {
    // UDP
    if (sendto(sock, rep.c_str(), rep.length(), 0, addr->ai_addr, addr->ai_addrlen) == -1)
        throw runtime_error("Error sending UDP reply message.");
}

void handle_request(int sock, string req) {
    string comm, PLID;
    stringstream ss(req);
    ss >> comm;
    // Start
    if (comm == "SNG") {
        ss >> PLID;
        handle_request_start(PLID, sock, addr);
        // reply: RSG status [n_letters max_errors]
    }
    // Play
    else if (comm == "PLG") {
        string letter;
        int trial;
        ss >> PLID >> letter >> trial;
        handle_request_play(PLID, letter, trial, sock, addr);
        return; //TODO: rm line
    }
    // Guess
    else if (comm == "PWG") {
        return; //TODO: rm line
    }
    // Quit/Exit
    else if (comm == "QUT") {
        return; //TODO: rm line
    }
    // Reveal
    else if (comm == "REV") {
        return; //TODO: rm line
    }

    return;
}

void handle_request_start(string PLID, int sock, addrinfo *addr) {
    char *fname = NULL;
    string res = "RSG";
    // "NOK": The player has an ongoing game
    // TODO: must check whether the game is ongoing or not
    if (find_active_game(PLID) != "") {
        res += " NOK";
        write_to_socket(sock, res, addr);
    }
    // "OK": The player has started a game
    else {
        res += " OK " + create_game_file(PLID);
        write_to_socket(sock, res, addr);
    }
    return;
}

void handle_request_play(string PLID, string letter, int trial, int sock, addrinfo *addr) {
    string filename = find_active_game(PLID);    
    //check if player has ongoing game
    if (filename == "") {
        // send back "RLG ERR trial 
        return;
    }

    // open game file for player
    ifstream game_file;
    string line, word, n_positions = find_letters(letter[0], word);
    game_file.open(filename/*, ios::in*/);
    getline(game_file, line);
    stringstream ss(line);
    ss >> word;
    // check if player has already played this letter
    string code, play, tr;
    bool dup;
    while(getline(game_file, line)) {
        stringstream ss(line);
        ss >> code >> play;
        if (code == "T" && play == letter && stoi(tr) != trial) {
            // send back "RLG DUP trial"
        } // I don't think the else case is needed
    }
    // send back "RLG OK trial" + n_positions
    return;
}

// TODO put in another section
string find_letters(char letter, string word) {
    string positions;

    for (int i = 0; i < word.length(); i++) {
        if (word[i] == letter) {
            positions += to_string(i) + " ";
        }
    }
    return positions;
}

// ============================================ Filesystem ===============================================

string create_game_file(string PLID) {
    ofstream file;
    string info, word, hint, fname = "games/GAME_" + PLID + ".txt";
    int numLetters, maxErrors;
    // TODO: is this working or should we use fopen, fwrite?
    // TODO: add exception (could not open file)

    // Choose word and write it and its hint file to the game file
    string info = get_word(words_file);
    file.open(fname, ios::in);
    file << info;
    file.close();

    // Set the number of maximum errors for the selected word
    stringstream(info) >> word >> hint;
    numLetters = word.length();
    if (numLetters <= 6)
        maxErrors = 7;
    else if (numLetters <= 10)
        maxErrors = 8;
    else
        maxErrors = 9;
    return to_string(numLetters) + " " + to_string(maxErrors);
}

string get_word(string words_file) {
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
    struct dirent **filelist;
    string fname, dname = "games/" + PLID + "/";
    int numEntries = scandir(dname.c_str(), &filelist, 0, alphasort);
    if (numEntries <= 0)
        return "";
    else {
        while (numEntries--) {
            if (filelist[numEntries]->d_name[0] != '.') {
                fname = dname + filelist[numEntries]->d_name;
                break;
            }
        }
    }
    return fname;
}

scorelist* find_top_scores() {
    struct dirent **filelist;
    struct scorelist *list;
    string fname, dname = "scores/";
    int i = 0, numEntries = scandir(dname.c_str(), &filelist, 0, alphasort);
    FILE *fp;
    if (numEntries < 0)
        return NULL;
    else {
        while (numEntries--) {
            if (filelist[numEntries]->d_name[0] != '.') {
                fname = dname + filelist[numEntries]->d_name;
                if (fp != NULL) {
                    fscanf(fp, "%d %s %s %d %d", &list->score[i], list->PLID[i], list->word[i], &list->numSuccess[i], &list->numTrials[i]);
                    fclose(fp);
                    ++i;
                }
            }
            if (i == 10)
                break;
        }
    }
    list->numScores = i;
    return list;
}