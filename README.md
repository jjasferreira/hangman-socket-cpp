 # hangman-socket-cpp

IST Computer Networks course's project (LEIC-A 2022/2023). Made by João Cardoso (ist199251) and José João Ferreira (ist199259).

---

## Brief description

This is an implementation of a hangman game, which consists in a client/server application, where communication between server and client is achieved by using both UDP and TCP sockets.

We opted to write the code in C++, in order to make use of some of its higher level functions, especially when it comes to input/string parsing. The `player` and `gs` executables are both compiled using the provided `Makefile` and represent the Player and the Game Server, respectively.

We have decided to strictly follow the project specifications. In cases where that was not possible, we tried to emulate the behavior of the server running on `tejo.tecnico.ulisboa.pt:59000`. In the rare situations where that was not possible, we implemented the specifications according to our best judgment.

In the `aux.h` file, we have put the auxiliary functions that are made use of in both the client and server files, since the project dimension did not, in our opinion, call for further division into more files, which would decrease readability. We also rewrote portions of the auxiliary functions that were provided so that they would be more in line with the C++ programming style.

Sockets are particularly useful to processes running separately and that need to communicate with each other, exchanging data.
You can run the **Player** and **Game Server** wherever you want, as long as you know the IP of the machine running the **Game Server** and establish a known port.

---

## Disclaimer

When testing our **Game Server** executable with the provided scripts hosted in `tejo.tecnico.ulisboa.pt:59000`, we often encountered some communication errors when analyzing the HTML. However, after some modifications, this problem seems to have subsided, although we cannot be sure it is gone entirely. This might have been happening because the reply to a request never made it to the **Player** and therefore was not processed by it, or their next request is stuck in the socket.

Either way, we believe this quirk was not due to any mistake on our part, seeing as we ensure on our side that the server reply is sent. In any client/server application, the onus falls on the client to ensure that the server reply did not get lost, by resending the request if it is lost. This logic cannot be implemented, of course, server-side, because the server has no way of knowing whether the packet was lost or not if the player doesn't resend the request.

---

## How to use

Making use of Domain Names/IPs and Ports, you can:
- run both the **Player** and the **Game Server** locally;
- run the **Player** locally and connect to an external **Game Server**;
- run the **Player** externally and connect to a local **Game Server**.

1. In order to run the **Player** and **Game Server**, firstly, you need to compile the code:

```bash
make
```

2. Then, you need to start the **Game Server** with the set of words and hints provided in the file `server/words.txt`, run:

```bash
./gs words.txt [-p gsPort] [-v]
```

If you wish to change the set of words being used, you can edit them manually. However, be aware that every hint associated with a word must have its own image file in the `server/images`.
The defaut port being used for the UDP and TCP sockets is `58092`. To use a custom port for communincation, use the `-p` flag and specify it in the _gsPort_ field.
To operate the **Game Server** in verbose mode (i.e. to have the **Player** requests being printed to the terminal), make use of the flag `-v`.

3. You can execute the **Player** by running:

```bash
./player [-n gsIP] [-p gsPort]
```

Both the `-n` and `-p` flags are optional. The first overrides the default **Game Server** IP (`localhost`, for games running on the same machine) with a custom one. In order to have the executables running on different machines, you need to specify the IP in _gsIP_ (e.g. `tejo.tecnico.ulisboa.pt`).
To change the default port, `58092`, as seen in the previous command, specify it in the _gsPort_ field.

4. The available game commands are presented to the **Player**, when they execute the file, and are as follows:

 - `sg <PLID>` or `start <PLID>` - starts a new game;
 - `pl <letter>` or `play <letter>` - submits the letter as a new play;
 - `gw <word>` or `guess <word>` - submits the word as a guess;
 - `sb` or `scoreboard` - shows the top 10 scores;
 - `h` or `hint` - retrieves an image file containing a hint for the word to be guessed;
 - `st` or `state` - displays the current game state, or the game state of the last game played by the player;
 - `rev` or `reveal` - reveals the current word (counts as quitting);
 - `quit` - terminates the current game;
 - `exit` - terminates the current game and the player application.