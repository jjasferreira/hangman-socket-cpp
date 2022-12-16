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
    // delete IP;
    return server;
}

int create_socket(string prot) {
    int type = (prot == "tcp") ? SOCK_STREAM : SOCK_DGRAM;
    int sock = socket(AF_INET, type, 0);
    if (sock == -1)
        throw runtime_error("Error opening " + prot + " socket.");
    return sock;
}

string request(int sock, addrinfo *addr, string req) {
    struct sockaddr_in address;
    char buffer[1024];
    
    // UDP connection request
    if (addr->ai_socktype == SOCK_DGRAM) {
        // Connect and send a message
        if (sendto(sock, req.c_str(), req.length(), 0, addr->ai_addr, addr->ai_addrlen) == -1)
            throw runtime_error("Error sending UDP request message.");
        // Receive a message
        socklen_t addrlen = sizeof(address);
        if (recvfrom(sock, buffer, 1024, 0, (struct sockaddr*) &address, &addrlen) == -1)
            throw runtime_error("Error receiving UDP reply message.");
    }
    // TCP connection request
    else if (addr->ai_socktype == SOCK_STREAM) {
        // Connect to the server
        if (connect(sock, addr->ai_addr, addr->ai_addrlen) == -1)
            throw runtime_error("Error connecting to server.");
        // Send a message
        if (write(sock, req.c_str(), req.length()) == -1)
            throw runtime_error("Error sending TCP request message.");
        // Receive a message
        if (read(sock, buffer, 1024) == -1)
            throw runtime_error("Error receiving TCP reply message.");
    }
    else
        throw runtime_error("Invalid socket type.");
    cout << "debug1 " << buffer << endl;
    return buffer;
}

string read_to_file(int sock, string mode) {
    string res, fname, fsize;
    ssize_t n;
    FILE *fd;
    char buffer[1024];
    bool first = true;

    // Read from socket to the file and print to terminal
    memset(buffer, 0, 1024);
    while ((n = read(sock, buffer, 1024)) > 0) {
        if (first) {
            stringstream(buffer) >> fname >> fsize;
            // Open/create the file with write permissions
            if ((fd = fopen(fname.c_str(), "w")) == NULL)
                throw runtime_error("Error opening/creating file.");
            // Skip the file name and file size
            char *p = buffer;
            int offset = fname.length() + fsize.length() + 2;
            p += offset;
            if (fwrite(p, sizeof(char), n - offset, fd) != (n - offset)*sizeof(char))
                throw runtime_error("Error writing to file.");
            if (mode == "print")
                cout << p;
            first = false;
            continue;
        }
        // Make sure the entire buffer is written to the file
        if (mode == "print")
            cout << buffer;
        if (fwrite(buffer, sizeof(char), n, fd) != n*sizeof(char))
            throw runtime_error("Error writing to file.");
    }
    fclose(fd);
    // delete fd;
    return fname + " " + fsize;
}