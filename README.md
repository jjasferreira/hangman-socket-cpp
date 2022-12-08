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

- _scoreboard/sb_
  - [x] player (GSB)
  - [ ] gs (RSB)
- _hint/h_
  - [x] player (GHL)
  - [ ] gs (RHL)
- _state/st_
  - [x] player (STA)
  - [ ] gs (RST)

---

## doubts

- "If there is no ongoing game for this player then status = FIN and the GS server responds with a file containing the summary of the most recently finished game for this player. Upon receiving this summary, the player displays the information and considers the current game as terminated." What game? There is no ongoing game.

- When we close the app with Ctrl+C mid-game and then run the executable again, it says that there is an ongoing game when we `start` and that we have quitted it successfully when we `quit`, but why does it say that there is an ongoing game again when we `start` again. Furthermore, when we `play`, it says there is no game. What is going on? Is this a bug?

- Change some string parameters to char\* to avoid using .c_str() or leave it as is?

- Should we have a dict with keys=udp,tcp and values=SOCK_DGRAM,SOCK_STREAM for connection functions scalability or leave it as is?

- What flags should we be using to compile the code in the Makefile?

- Because the scoreboard message reply is sent in two parts (RSG OK // fname fsize fdata) using TCP, what is the best way to extract the filename and filesize, given that we read all that inside a loop? Can we read 25 bytes outside the loop to get this info or should we do something inside the loop? Can the filesize help us somehow?

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
