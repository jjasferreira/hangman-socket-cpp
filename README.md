# hangman-socket-cpp

## todo

**udp**:

- _start/sg_
  - [x] player (SNG)
  - [ ] gs (RSG)
- _play/pl_
  - [x] player (PLG)
  - [ ] gs (RLG)
- _guess/gw_
  - [x] player (PWG)
  - [ ] gs (RWG)
- _quit_
  - [x] player (QUT)
  - [ ] gs (RQT)
- _rev_
  - [x] player (REV) (change to return OK instead of word)
  - [ ] gs (RRV)
- [x] _exit_

**tcp**:

- [ ] _scoreboard/sb_
- [ ] _hint/h_
- [ ] _state/st_

---

## doubts

- Should we keep the game boolean variable in the player side? Is it really useful?

- Should we have a dict with keys=udp,tcp and values=SOCK_DGRAM,SOCK_STREAM for connection functions scalability or leave it as is?

- What flags should we be using to compile the code in the Makefile?

- (ASKED) When any player input is incorrect (e.g. playing a letter without having an ongoing game, sending a PLID with 7 digits) should we send this to the GS and wait for an ERR or do a small check on that input in the player side? If it is the latter, should we throw an error and therefore terminate the program or just print the mistake to the terminal and continue the commmand handling loop?

- Should we use setsockopt(), select() or SIGNALS to have a timer in the reception of data from the server? In UDP as well or just in TCP?

- Do we create a new process entirely on the TCP server when the new connection socket is created or do we keep both sockets of the server running on the same process? If it is the first one, then do we kill the unused socket in each process (listenfd and connfd, as in the slides)?

---

## teacher tips

- If there is a game in progress when the player gives the command "exit" on the keyboard, the application will ask the server to end the game in progress. All sockets must be closed and memory freed before terminating the application.

- We should have a welcome message to indicate which commands are available and print the number of attempts left throughout the game

- We should indicate there has been an error by printing it when the player receives just ERR

- It is a good idea to detect SIGINT to terminate the game and release memory before exiting (only in C though?, how do we release memory in C++?)

- During the execution of the client and the server, we have to verify that the commands are well inserted (start, play, etc) and that the messages are also well formatted (SNG XXXXXX, RSG OK … …, etc)

- Implement a data reception confirmation mechanism on the UDP protocol (recvfrom is bad on itself. Teacher talked about the SELECT mechanism or having a timer: input multiplexing)

- fork() on the server, we have to delete the child proccesses after they are done (SIGPIPE KILL CHILD)

- Specifying which type of error is occuring when it comes to transporting data might be good

- Sometimes it may be needed to read() or write() multiple times to receive/send the full message - In the TCP protocol, when sending large data such as images, as the buffer size is much smaller than the file size, the file is divided into several pieces and on the other side, the client picks them up and joins them. (In the case of TCP, even when the entire message fits in the buffer, the TCP implementation may decide to split the message into several TCP segments, so we should always write and read in a cycle, checking what has already been written/read, to ensure that everything is written/read.)
