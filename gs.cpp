#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>

#define GN 66

using namespace std;
pair<string, string> get_word(string word_file);

int GSport = 58000 + GN;

int main(int argc, char* argv[]) {
    int fd;

    if (argc < 1) {
        throw invalid_argument("No word file specified");
    }
    else if (argc = 2) {
        string word_file = argv[1];
        string word, cat;
        /*DUVIDA há uma maneira melhor de fazer isto?*/
        pair<string, string> word_cat = get_word(word_file);
        word = word_cat.first;
        cat = word_cat.second;
        cout << word << endl << cat << endl;
    }
    else if (argc > 2) {
        GSport = atoi(argv[2]);
    }

    // open udp socket
    // fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        throw runtime_error("Error opening socket");
    }


    return 0;
}

pair<string, string> get_word(string word_file) {
    string line;
    vector<string> lines;
    string word, cat;
    int numLines = 0;

    srand(time(NULL));

    /*open file containing words*/
    ifstream input(word_file);
    while(getline(input, line)) {
        ++numLines;
        lines.push_back(line);
    }
    int random = rand() % numLines;
    stringstream(lines[random]) >> word >> cat;
    return {word, cat};
}