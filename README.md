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
  - [ ] player (QUT)
  - [ ] gs (RQT)
- _rev_
  - [ ] player (REV)
  - [ ] gs (RRV)
- [ ] _exit_

**tcp**:

- [ ] _scoreboard/sb_
- [ ] _hint/h_
- [ ] _state/st_

---

## doubts

- Do we have to create/choose the hint images?

- What flags should we be using to compile the code in the Makefile?

- When PLID is not digit-only or is of length bigger than 6, should we treat the error in the player level or in the GS level and return ERR/RSG NOK?

- When input is incorrect (e.g. PLID with 7 digits) should we throw an error (e.g.'std::invalid_argument') and therefore terminate the program or just print the mistake to the terminal and continue the commmand handling loop?

- Should we print a custom message for each different possible play scenario (e.g. DUP, WIN, ...) to the user terminal or only "Word: _ e _ o l", as in the project statement?

- Should we use recvfrom with a timer or select()? If so, how does select work?

- Should we check if a letter, word or PLID are valid according to the syntax or just send them anyway?

- Should we have a welcome message to indicate which commands are available for use or not?

- Are you expecting us to print the number of attempts left throughout the game?

- On the project statement, it says "OVR, if the word guess is not correct and there are no more attempts
  available (the maximum number of errors or the maximum number of trials was reached)". Where is this maximum number of trials spoken about exactly?

- Do we create a new process entirely on the TCP server when the new connection socket is created or do we keep both sockets of the server running on the same process? If it is the first one, then do we kill the unused socket in each process (listenfd and connfd, as in the slides)?

---

## teacher tips

- Implement a data reception confirmation mechanism on the UDP protocol (recvfrom is bad on itself. Teacher talked about the SELECT mechanism or having a timer: input multiplexing)

- Specifying which type of error is occuring when it comes to transporting data might be good

- Sometimes it may be needed to read() or write() multiple times to receive/send the full message
