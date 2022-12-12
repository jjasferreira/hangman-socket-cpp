#include <netdb.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

#define GN 92

// ======================================== Declarations ===========================================

struct addrinfo *addrUDP, *addrTCP; // addrinfo structs for UDP and TCP connections
string gsIP = "localhost";
string gsPort = to_string(58000 + GN);
bool verbose = false;

// Get the IP address of the given game server
addrinfo* get_server_address(string gsPort, string prot = "udp");
// Create sockets using IPv4 and the UDP or TCP protocol
int create_socket(string prot = "udp");

void handle_request_udp(char* buffer, int n);

// ============================================ Main ===============================================

int main(int argc, char* argv[]) {
    
    struct addrinfo hints, *res;
    socklen_t addrlen;
    struct sockaddr_in address;
    char buffer[1024];

    // Determine the words file and overwrite the Port (words_file [-p gsPort] [-v])
    if (argc < 1) // Exception
        throw invalid_argument("No words file specified");
    else
        string words_file = argv[1];
        // cout << "Words file: " << words_file << endl; // TODO: rm line
    for (int i = 2; i < argc; ++i) {
        if (!strcmp(argv[i], "-p"))
            gsPort = argv[i + 1];
        else if (!strcmp(argv[i], "-v"))
            verbose = true;
    }
    // Get the Game Server addresses for UDP and TCP connections
    addrUDP = get_server_address(gsPort);
    addrTCP = get_server_address(gsPort, "tcp");
    

    addrlen = sizeof(address);
    
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
            if((nsock = accept(sock, (struct sockaddr*) &address, &addrlen)) == -1)
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

    
    // Parent process listens to UDP connections
    } else {
        // Create an UDP socket 
        int n;

        int udp_sock = create_socket();

        if (bind(udp_sock, res->ai_addr, res->ai_addrlen) < 0)
            throw runtime_error("Error binding UDP socket");
        while (true) {
            n = recvfrom(udp_sock, (char *)buffer, 1024, 0, (struct sockaddr *) &address, &addrlen);
            if (n < 0)
                throw runtime_error("Error reading from UDP socket");
            cout << "Received UDP message: " << buffer << endl;
            // handle_request_udp(buffer, n); // TODO implement   
            close(udp_sock)
        }
    }
    return 0;
}

// ==================================== Auxiliary Functions ========================================

int create_socket(string prot) {
    int type = (prot == "tcp") ? SOCK_STREAM : SOCK_DGRAM;
    int sock = socket(AF_INET, type, 0);
    if (sock == -1)
        throw runtime_error("Error opening " + prot + " socket.");
    return sock;
}

addrinfo* get_server_address(string gsPort, string prot) {
    struct addrinfo hints, *server;
    int type = (prot == "tcp") ? SOCK_STREAM : SOCK_DGRAM;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = type;
    hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(NULL, gsPort.c_str(), &hints, &server) != 0)
        throw runtime_error("Error getting address info.");
    return server;
}

addrinfo* get_server_address(string gsIP, string gsPort, string prot) {
    struct addrinfo hints, *server;
    int type = (prot == "tcp") ? SOCK_STREAM : SOCK_DGRAM;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = type;
    if (gsIP == NULL)
        hints.ai_flags = AI_PASSIVE;
    if (getaddrinfo(gsIP.c_str(), gsPort.c_str(), &hints, &server) != 0)
        throw runtime_error("Error getting address info.");
    return server;
}