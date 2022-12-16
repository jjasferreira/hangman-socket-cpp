# hangman-socket-cpp

## todo

**udp**:

- _start/sg_: [ ] RSG
- _play/pl_: [ ] RLG
- _guess/gw_: [ ] RWG
- _quit_: [ ] RQT
- _rev_: [ ] RRV (change to return OK instead of word, also see REV)
- _exit_: [ ] (release memory?)

**tcp**:

- _scoreboard/sb_: [ ] RSB
- _hint/h_: [ ] RHL
- _state/st_: [ ] RST

- docstrings for functions so that when the teacher hover them, they see the description
- RCOMP command for debugging?

---

## doubts

- Are we supposed to select the words from the word file in order as is done is Tejo or can we select them randomly?

- "If there is no ongoing game for this player then status = FIN and the GS server responds with a file containing the summary of the most recently finished game for this player. Upon receiving this summary, the player displays the information and considers the current game as terminated." What game? There is no ongoing game.

- When we close the app with Ctrl+C mid-game and then run the executable again, it says that there is an ongoing game when we `start` and that we have quitted it successfully when we `quit`, but why does it say that there is an ongoing game again when we `start` again. Furthermore, when we `play`, it says there is no game. What is going on? Is this a bug?

- Change some string parameters to char\* to avoid using .c_str() or leave it as is?

- Should we have a dict with keys=udp,tcp and values=SOCK_DGRAM,SOCK_STREAM for connection functions scalability or leave it as is?

- Because the scoreboard message reply is sent in two parts (RSG OK // fname fsize fdata) using TCP, what is the best way to extract the filename and filesize, given that we read all that inside a loop? Can we read 25 bytes outside the loop to get this info or should we do something inside the loop? Can the filesize help us somehow?

- Should we use setsockopt(), select() or SIGNALS to have a timer in the reception of data from the server? In UDP as well or just in TCP?

- Do we create a new process entirely on the TCP server when the new connection socket is created or do we keep both sockets of the server running on the same process? If it is the first one, then do we kill the unused socket in each process (listenfd and connfd, as in the slides)?

---

## teacher tips

- Ignore SIGPIPE player (write on unavailable socket)

- There is an initial fork() and the child process listens for TCP connections, while the parent process listens for UDP connections. In the parent, it is not necessary to open new processes, since in UDP there is a queue. In the child, it will be necessary to open a child process for each file transfer, which is then closed.

- If there is a game in progress when the player gives the command "exit" on the keyboard, the application will ask the server to end the game in progress. All sockets must be closed and memory freed before terminating the application.

- We should indicate there has been an error by printing it when the player receives just ERR

- It is a good idea to detect SIGINT to terminate the game and release memory before exiting (only in C though?, how do we release memory in C++?)

- Implement a data reception confirmation mechanism on the UDP protocol (recvfrom is bad on itself. Teacher talked about the SELECT mechanism or having a timer: input multiplexing)

- fork() on the server, we have to delete the child processes after they are done (SIGPIPE KILL CHILD)
