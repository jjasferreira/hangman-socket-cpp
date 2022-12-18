# hangman-socket-cpp

## todo

**udp**:

- _start/sg_: [ ] RSG
- _play/pl_: [ ] RLG
- _guess/gw_: [ ] RWG
- _quit_: [ ] RQT
- _rev_: [ ] RRV
- _exit_: [ ] (release memory? delete pointers and tcp conns)

**tcp**:

- _scoreboard/sb_: [ ] RSB
- _hint/h_: [ ] RHL
- _state/st_: [ ] RST

- docstrings for functions so that when the teachers hover them, they see the description?
- RCOMP command for debugging?

---

- se player receber invalid trial number o ctrl c deixa de funcionar

## doubts

- [ASKED] Should we use setsockopt() to have a timer in the reception of data from the server? Yes, in the player, have timers in UDP sockets. In the player side, we should have a timer to close TCP sockets after they've sent everything >> kill child process >> wait in parent process >> exit server

- [ASKED] Originally, the RRV reply message could have status (OK, ERR) or present the word. But now it says it only sends the word back or no reply message at all. "No reply means that there is no ongoing game". How do we deal with this? With timers?

- How do we not print garbage when in verbose mode in the server? How can we not send garbage?

- "If there is no ongoing game for this player then status = FIN and the GS server responds with a file containing the summary of the most recently finished game for this player. Upon receiving this summary, the player displays the information and considers the current game as terminated." What game? There is no ongoing game.

- When we close the app with Ctrl+C mid-game and then run the executable again, it says that there is an ongoing game when we `start` and that we have quitted it successfully when we `quit`, but why does it say that there is an ongoing game again when we `start` again. Furthermore, when we `play`, it says there is no game. What is going on? Is this a bug?

- Is there any problem in using strings or should we use char\*?

- Because the scoreboard message reply is sent in two parts (RSG OK // fname fsize fdata) using TCP, what is the best way to extract the filename and filesize, given that we read all that inside a loop? Can we read 25 bytes outside the loop to get this info or should we do something inside the loop? Can the filesize help us somehow?

- Do we create a new process entirely on the TCP server when the new connection socket is created or do we keep both sockets of the server running on the same process? If it is the first one, then do we kill the unused socket in each process (listenfd and connfd, as in the slides)?

---

## teacher tips

- We have to close all TCP processes inside the Ctrl+C signal handler (using wait/waitpid?)

- We must verify that, on the server side, the check for INV comes before NOK in plays and guesses (and after that test nc -u on the server to confirm if the teachers are right)

- Ignore SIGPIPE player (write on unavailable socket)

- There is an initial fork() and the child process listens for TCP connections, while the parent process listens for UDP connections. In the parent, it is not necessary to open new processes, since in UDP there is a queue. In the child, it will be necessary to open a child process for each file transfer, which is then closed.

- Implement a data reception confirmation mechanism on the UDP protocol (recvfrom is bad on itself. Teacher talked about the SELECT mechanism or having a timer: input multiplexing)

- fork() on the server, we have to delete the child processes after they are done (SIGPIPE KILL CHILD)
