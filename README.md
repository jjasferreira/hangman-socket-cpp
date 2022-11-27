# hangman-socket-cpp

## todo

**udp**:

- _start/sg_
  - [x] player (SNG)
  - [ ] gs (RSG)
- _play/pl_
  - [ ] player (PLG)
  - [ ] gs (RLG)
- _guess/gw_
  - [ ] player (PWG)
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

- When PLID is not digit-only or is of length bigger than 6, should we treat the error in the player level or in the GS level and return NOK?

- When input is incorrect (e.g. PLID with 7 digits) should we throw an instance of 'std::invalid_argument' and therefore terminate the program or just printf() the mistake and continue the commmand handling loop?

---

## teacher tips

- All UDP and TCP connections are closed immediately after the command has been run

- Implement a data reception confirmation mechanism on the UDP protocol (recvfrom is bad on itself. Teacher talked about the SELECT mechanism or having a timer: input multiplexing)

- Specifying which type of error is occuring when it comes to transporting data might be good

- Sometimes it may be needed to read() or write() multiple times to receive/send the full message
