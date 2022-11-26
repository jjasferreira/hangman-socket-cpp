#include <netdb.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>

using namespace std;

#define GN 0    // TODO: change group number

// ======================================== Declarations ===========================================

pair<string, string> get_word(string word_file);

string gsPort = to_string(58000 + GN);

// ============================================ Main ===============================================

int main(int argc, char* argv[]) {
    int sock;

    if (argc < 1)
        throw invalid_argument("No word file specified");
    else if (argc == 2) {
        string word_file = argv[1];
        string word, cat;
        // TODO: is there a better way to do this?
        pair<string, string> word_cat = get_word(word_file);
        word = word_cat.first;
        cat = word_cat.second;
        cout << word << endl << cat << endl;
    }
    else if (argc > 2)
        gsPort = argv[2];

    // Open UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
        throw runtime_error("Error opening socket");

    return 0;
}

// ==================================== Auxiliary Functions ========================================

pair<string, string> get_word(string word_file) {
    string line, word, cat;
    vector<string> lines;
    int numLines = 0;
    srand(time(NULL));

    // Open file containing words
    ifstream input(word_file);
    while(getline(input, line)) {
        ++numLines;
        lines.push_back(line);
    }
    int random = rand() % numLines;
    stringstream(lines[random]) >> word >> cat;
    return {word, cat};
}