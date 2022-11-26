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

---

## teacher tips

- Implement a data reception confirmation mechanism on the UDP protocol (recvfrom is bad on itself. Teacher talked about the SELECT mechanism or having a timer: input multiplexing)

- Specifying which type of error is occuring when it comes to transporting data might be good

- Sometimes it may be needed to read() or write() multiple times to receive/send the full message
