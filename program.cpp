#include <iostream>
#include <string>
#include <sstream>

using namespace std;

int main() {
    string line;
    string parameter;
    getline(cin, line);
    stringstream ss(line);
    while(ss >> parameter) {
        cout << parameter << endl;
    }
    cout << line << endl;
    return 0;
}