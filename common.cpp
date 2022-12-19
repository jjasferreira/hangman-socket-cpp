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
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv)) < 0)
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
    char buffer[64];
    ssize_t n;

    // Receive a message
    socklen_t addrlen = sizeof(address);
    n = recvfrom(sock, buffer, 64, 0, (struct sockaddr*) &address, &addrlen);
    if (n == -1)
        throw runtime_error("Error receiving UDP reply message.");
    buffer[n] = '\0';
    return buffer;
}

string read_to_file(int sock, string mode) {
    string fname, fsize;
    FILE *fd;
    ssize_t n, size;
    char buffer[1024], c;

    // Read filename and filesize from socket and check if they are valid
    while ((n = read(sock, &c, 1)) > 0 && c != ' ')
        fname += c;
    if (!is_valid_filename(fname)) {    // Exception
        cout << "The file name is invalid." << endl << endl;
        return "FNERR";
    }
    while ((n = read(sock, &c, 1)) > 0 && c != ' ')
        fsize += c;
    if (!is_valid_filesize(fsize)) {    // Exception
        cout << "The file size is invalid." << endl << endl;
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