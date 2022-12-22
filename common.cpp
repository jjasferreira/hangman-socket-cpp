#include "common.h"

bool is_valid_port(const string &arg) {
    return all_of(arg.begin(), arg.end(), ::isdigit) && (stoi(arg) >= 1024) && (stoi(arg) <= 65536);
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

bool is_valid_integer(const string &arg) {
    return all_of(arg.begin(), arg.end(), ::isdigit) && (arg.length() > 0);
}

bool is_valid_filename(const string &arg) {
    return (arg.length() <= 24) && all_of(arg.begin(), arg.end(), [](char c) { return isalnum(c) || c == '-' || c == '_' || c == '.'; });
}

bool is_valid_filesize(const string &arg) {
    return all_of(arg.begin(), arg.end(), ::isdigit) && (arg.length() > 0) && (stoi(arg) <= 1073741824);
}

addrinfo* get_server_address(string gsIP, string gsPort, string prot) {
    struct addrinfo hints, *server;
    char *IP = (char *) gsIP.c_str();
    int type = (prot == "tcp") ? SOCK_STREAM : SOCK_DGRAM;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = type;
    // Server IP is "self" if it is a server
    if (gsIP == "self") {
        hints.ai_flags = AI_PASSIVE;
        IP = NULL;
    }
    if (getaddrinfo(IP, gsPort.c_str(), &hints, &server) != 0)
        throw runtime_error("Error getting address info.");
    return server;
}

int create_socket(addrinfo *addr, string prog) {
    int sock = socket(AF_INET, addr->ai_socktype, 0);
    if (sock == -1) {
        string prot = (addr->ai_socktype == SOCK_DGRAM) ? "UDP" : "TCP";
        throw runtime_error("Error opening " + prot + " socket.");
    }
    // Set a timer of 5 seconds if the socket is UDP and belongs to the Player
    if (addr->ai_socktype == SOCK_DGRAM && prog == "player") {
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
            throw runtime_error("Error setting socket options");
    }
    return sock;
}

void write_to_socket(int sock, addrinfo *addr, string rep) {
    // UDP connection request
    if (addr->ai_socktype == SOCK_DGRAM) {
        // Connect and send a message
        if (sendto(sock, rep.c_str(), rep.length(), 0, addr->ai_addr, addr->ai_addrlen) == -1)
            throw runtime_error("Error sending UDP request message.");
    }
    // TCP connection request
    else if (addr->ai_socktype == SOCK_STREAM) {
        // Connect to the server
        if (connect(sock, addr->ai_addr, addr->ai_addrlen) == -1)
            throw runtime_error("Error connecting to server.");
        // Send a message
        if (write(sock, rep.c_str(), rep.length()) == -1)
            throw runtime_error("Error sending TCP request message.");
    }
    else    // Exception
        throw runtime_error("Invalid address type.");
}

string read_from_socket(int sock) {
    struct sockaddr_in address;
    socklen_t addrLen = sizeof(address);
    char buffer[1024];
    ssize_t n;

    // Receive a message
    n = recvfrom(sock, buffer, 1024, 0, (struct sockaddr*) &address, &addrLen);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            throw runtime_error("Timeout receiving UDP reply message.");
        else
            throw runtime_error("Error receiving UDP reply message.");
    }
    buffer[n] = '\0';
    return buffer;
}

string read_to_file(int sock, string mode) {
    string fname, fsize;
    FILE *fd;
    ssize_t n, size;
    char buffer[1024], c;

    // Read filename and filesize from socket and check if they are valid
    while ((n = read(sock, &c, 1)) > 0 && c != ' ' && c != '\0')
        fname += c;
    if (!is_valid_filename(fname)) {    // Exception
        cout << "The file name is invalid: " << fname << endl << endl;
        return "FNERR";
    }
    while ((n = read(sock, &c, 1)) > 0 && c != ' ' && c != '\0')
        fsize += c;
    if (!is_valid_filesize(fsize)) {    // Exception
        cout << "The file size is invalid: " << fsize << endl << endl;
        return "FSERR";
    }
    size = stoi(fsize);
    // Open/create the file with write permissions
    if ((fd = fopen(fname.c_str(), "w")) == NULL)
        throw runtime_error("Error opening/creating file.");
    // Read data from socket to the file and print to terminal if mode is "print"
    memset(buffer, 0, 1024);
    while ((n = read(sock, buffer, 1024)) > 0 && size > 0) {
        if (fwrite(buffer, sizeof(char), n, fd) != n*sizeof(char))
            throw runtime_error("Error writing to file.");
        size -= n;
        if (mode == "print")
            cout << buffer;
    }
    // Close the file and print the result
    fclose(fd);
    return fname + " " + fsize;
}

string get_word(ifstream &wordsFile) {
    string line, word, hint;
    if (!wordsFile.is_open())
        throw runtime_error("Error obtaining a word from the words file.");
    // If the pointer is in the end, move it to the top, without clearing the file
    if (wordsFile.peek() == EOF)
        wordsFile.seekg(0, ios::beg);
    getline(wordsFile, line);
    stringstream(line) >> word >> hint;
    return word + " " + hint;
}

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

string create_game_file(string PLID, ifstream &wordsFile) {
    int numLetters, maxErrors;
    string info, word, hint;
    string filePath = "server/games/GAME_" + PLID + ".txt";
    ofstream file(filePath, ios::in | ios::out | ios::trunc);
    if (!file.is_open())    // Exception
        throw runtime_error("Could not create game file");

    // Choose word and write it and its hint file to the game file
    info = get_word(wordsFile);
    file << info << endl;
    file.close();
    // Set the number of maximum errors for the selected word
    stringstream(info) >> word >> hint;
    numLetters = word.length();
    maxErrors = calculate_max_errors(numLetters);
    return to_string(numLetters) + " " + to_string(maxErrors);
}

bool has_moves(string gamePath) {
    ifstream gameFile(gamePath);
    string line;
    // If we are able to read a line, check if there is another line
    if (getline(gameFile, line)) {
        getline(gameFile, line);
        return (line.length() > 0);
    }
    return false;
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
    string statePath, gamePath = get_active_game(PLID);
    string title = "Active game found for player " + PLID;
    char termState;
    bool activeGame = true;
    // If there is no active game, find the last finished game
    if (gamePath == "") {
        activeGame = false;
        gamePath = get_last_game(PLID);
        if (gamePath == "") // Exception
            throw runtime_error("No games (active or finished) found for this Player.");
        title = "Last finalized game for player " + PLID;
        // Get the termination state of the last game
        size_t lastUndScPos = gamePath.find_last_of('_');
        termState = gamePath.substr(lastUndScPos + 1)[0];
    }
    // Open the game file and extract the word and hint file
    fstream gameFile(gamePath);
    if (!gameFile.is_open()) // Exception
        throw runtime_error("Error opening game file.");
    string line, word, hint;
    getline(gameFile, line);
    stringstream(line) >> word >> hint;
    // Create a state file with a unique name and write the state to it
    statePath = "server/temp/STATE_" + PLID + ".txt";
    ofstream stateFile(statePath, ios::out | ios::trunc);
    stateFile << title << endl;
    stateFile << "Word: " << word << "; Hint file: " << hint << endl;
    // Compute transaction count
    int transCount = 0;
    while (getline(gameFile, line)) {
        if (line != "")
            transCount++;
    }
    stateFile << "--- Transactions found: " << transCount << " ---" << endl;
    gameFile.close();
    gameFile.open(gamePath);
    // Skip the first line
    getline(gameFile, line);
    // Set a set with letters from word and another with guessed letters
    set<char> letters;
    for (char c : word)
        letters.insert(c);
    set<char> guessed;
    // Iterate through the transactions and update the state
    string code, play;
    while (getline(gameFile, line)) {
        stringstream(line) >> code >> play;
        if (code == "T") {
            // If the letter is in the word, add it to the set of guessed letters
            if (letters.find(play[0]) != letters.end()) {
                stateFile << "Letter trial: " << play << " - TRUE" << endl;
                guessed.insert(play[0]);
            }
            else
                stateFile << "Letter trial: " << play << " - FALSE" << endl;
        }
        else if (code == "G")
            stateFile << "Word guess: " << play << endl;
    }
    // Build wordProgress indicator if there is an active game
    if (activeGame) {
        string wordProgress;
        for (char c : word) {
            if (guessed.find(c) != guessed.end())
                wordProgress += c + " ";
            else
                wordProgress += "_ ";
        }
        stateFile << "Solved so far: " << wordProgress << endl;
    }
    else {
        map<char, string> termStates = {{'W', "WIN"}, {'F', "FAIL"}, {'Q', "QUIT"}};
        stateFile << "Termination: " << termStates[termState] << endl;
    }
    return statePath;
}

void create_score_file(string PLID, string gamePath) {
    fstream gameFile(gamePath);
    if (!gameFile.is_open())
        cout << "Error opening game file" << endl;
    // Get the word from the game file and set a set of letters
    string word, line;
    getline(gameFile, line);
    stringstream(line) >> word;
    set<char> letters;
    for (char c : word)
        letters.insert(c);
    // Compute the score based on the ratio of successful trials
    int score = 0, numSuccess = 0, numTrials = 0;
    string code, play;
    while (getline(gameFile, line)) {
        numTrials++;
        stringstream(line) >> code >> play;
        if (code == "G" && play == word)
            numSuccess++;
        else if (code == "T" && letters.find(play[0]) != letters.end())
            numSuccess++;
    }
    gameFile.close();
    // The score file name should be score_PLID_DDMMYYYY_HHMMSS.txt
    if (numTrials == 0)
        throw runtime_error("Could not compute score for game with no trials.");
    score = numSuccess * 100 / numTrials;
    time_t t = time(0);
    tm* now = localtime(&t);
    string date = to_string(now->tm_year + 1900) + to_string(now->tm_mon + 1) + to_string(now->tm_mday)
        + "_" + to_string(now->tm_hour) + to_string(now->tm_min) + to_string(now->tm_sec);
    string scorePath = "server/scores/" + to_string(score) + "_" + PLID + "_" + date + ".txt";
    // Open the score file and write the score
    ofstream scoreFile(scorePath, ios::out | ios::in | ios::trunc);
    if (!scoreFile.is_open())    // Exception
        throw runtime_error("Could not create score file.");
    scoreFile << to_string(score) << " " << PLID << " " << word << " " << to_string(numSuccess) << " " << to_string(numTrials) << endl;
    scoreFile.close();
}

void move_to_past_games(string PLID, string status) {
    string gamePath = get_active_game(PLID);
    if (gamePath == "") // Exception
        throw runtime_error("No active game found for this PLID.");
    // Create a score file based on this game
    if (status == "win")
        create_score_file(PLID, gamePath);
    map<string, string> statusCodes = {{"win", "W"}, {"fail", "F"}, {"quit", "Q"}};
    // Get the current date and time
    time_t t = time(0);
    tm* now = localtime(&t);
    string date = to_string(now->tm_year + 1900) + to_string(now->tm_mon + 1) + to_string(now->tm_mday)
        + "_" + to_string(now->tm_hour) + to_string(now->tm_min) + to_string(now->tm_sec);
    // Apply the commands: create a directory and move the file there
    string mkdirComm = "mkdir -p server/games/" + PLID;
    string mvComm = "mv " + gamePath + " server/games/" + PLID + "/" + date + "_" + statusCodes[status] + ".txt";
    system(mkdirComm.c_str());
    system(mvComm.c_str());
    return;
}

long get_file_size(string filePath) {
    struct stat statBuf;
    int res = stat(filePath.c_str(), &statBuf);
    return res == 0 ? statBuf.st_size : -1;
}

string get_active_game(string PLID) {
    string filePath = "server/games/GAME_" + PLID + ".txt";
    ifstream file(filePath);
    if (!file.is_open())
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