#ifndef COMMON_H
#define COMMON_H

#include <unistd.h>
#include <netdb.h>
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <cctype>
#include <vector>
#include <set>
#include <map>
#include <iomanip>
#include <signal.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

using namespace std;

#define GN 92

// Struct that can save the top 10 scores of a Game Server
struct scorelist {
    int score[10];
    string PLID[10];
    string word[10];
    int numSuccess[10];
    int numTrials[10];
    int numScores;
};

// Checks if the given string is composed of
bool is_valid_port(const string &arg);      // digits only and is between 1024 and 65535
bool is_valid_plid(const string &arg);      // digits only and is of length 6
bool is_valid_letter(const string &arg);    // letters only and is of length 1
bool is_valid_word(const string &arg);      // letters only and is of length 1 or more
bool is_valid_integer(const string &arg);   // digits only and is of length 1 or more
bool is_valid_filename(const string &arg);  // letters and digits and "-", "_" or "." only and is of length 24 or less
bool is_valid_filesize(const string &arg);  // digits only and is equal to 1GiB or less

// Get the IP address of the given game server
addrinfo* get_server_address(string gsIP, string gsPort, string prot = "udp");

// Create sockets using IPv4 and the UDP or TCP protocol
int create_socket(addrinfo *addr, string prog = "player");

// Sends a message through the UDP or TCP socket
void write_to_socket(int sock, addrinfo *addr, string rep);

// Reads a message from an UDP socket
string read_from_socket(int sock);

// Reads the response from the server and writes it to a file
string read_to_file(int sock, string mode = "quiet");

// Get the next word on the opened words file
string get_word(ifstream &wordsFile);

// Get a string with the number of positions and respective indexes of a given letter in a word
string get_letter_positions(string word, char letter);

// Get the maximum number of errors allowed for a word of the given size
int calculate_max_errors(int len);

// Create a new active game file for the given Player
string create_game_file(string PLID, ifstream &wordsFile);

// Check if the game file has got moves
bool has_moves(string gamePath);

// Create a new file to store the top scores of the server
string create_scoreboard_file(scorelist *sb);

// Return the image hint file path for the word being guesses by the Player
string get_hint_file(string PLID);

// Create a new state of game file for the given Player
string create_state_file(string PLID);

// Create a new score file for the given Player
void create_score_file(string PLID, string gamefile);

// Move the active game file to the past games folder and rename it
void move_to_past_games(string PLID, string status);

// Get the size of a file on the given path
long get_file_size(string filePath);

// Return the path of the active game file of the given Player
string get_active_game(string PLID);

// Return the path of the last finished game file of the given Player
string get_last_game(string PLID);

// Return the struct scorelist filled with the top 10 scores of the Server
scorelist* get_top_scores();

#endif