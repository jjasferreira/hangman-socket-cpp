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

- "If there is no ongoing game for this player then status = FIN and the GS server responds with a file containing the summary of the most recently finished game for this player. Upon receiving this summary, the player displays the information and considers the current game as terminated." What game? There is no ongoing game.

- When we close the app with Ctrl+C mid-game and then run the executable again, it says that there is an ongoing game when we `start` and that we have quitted it successfully when we `quit`, but why does it say that there is an ongoing game again when we `start` again. Furthermore, when we `play`, it says there is no game. What is going on? Is this a bug?

- If we create a new process on the TCP server when the new connection socket is created, then, do we kill the unused socket in each process (`listenfd` and `connfd`, as in the slides)?

---

## teacher tips

- In the player, have timers in UDP sockets. In the server, have a timer to close TCP sockets after they've sent everything >> kill child process >> wait in parent process >> exit server

- fgtets para state e para scoreboard, fwrite para hints (servidor)

- We have to close all TCP processes inside the Ctrl+C signal handler (using wait/waitpid?)

- We must verify that, on the server side, the check for INV comes before NOK in plays and guesses (and after that test nc -u on the server to confirm if the teachers are right)

- Ignore SIGPIPE player (write on unavailable socket)

- fork() on the server, we have to delete the child processes after they are done (SIGPIPE KILL CHILD)
