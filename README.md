# hangman-socket-cpp

IST Computer Networks course's project (LEIC-A 2022/2023). Made by [João Cardoso](<(https://github.com/joaoncardoso)>) and [José João Ferreira](https://github.com/jjasferreira).

---

## Brief description

This is an implementation of a Hangman Game, which consists in a client/server application, where communication between server and client is achieved by using both UDP and TCP sockets.

We opted to write the code in C++, in order to make use of some of its higher level functions, especially when it comes to input/string parsing. The `player` and `gs` executables are both compiled using the provided `Makefile` and represent the Player and the Game Server, respectively.

We have decided to strictly follow the project specifications. In cases where that was not possible, we tried to emulate the behavior of the server that was up while we were developing the project, which was `tejo.tecnico.ulisboa.pt:59000`. In the rare situations where that was not possible, we implemented the specifications according to our best judgment.

In the `auxiliary.h` file, we have put the auxiliary functions that are made use of in both the client and server files, since the project dimension did not, in our opinion, call for further division into more files, which would decrease readability. We also rewrote portions of the auxiliary functions that were provided so that they would be more in line with the C++ programming style.

Occasionally, we use the `server/temp` directory in order to have a place to store the scoreboard and state files before sending them. This directory is deleted when the server stops executing and the same applies to the active games (files starting with "GAME\_") left unfinished inside the directory `server/games`.

The hint images for the words in the words file must be stored inside the `server/images`.

To stop the **Game Server** from running, you must press `Ctrl+C` while in the terminal. This sends the _SIGINT_ signal, which is handled by a function that we defined to close the **Game Server** safely. The same logic applies to the executable of the **Player**.

Sockets are particularly useful to processes running separately and that need to communicate with each other, exchanging data.
You can run the **Player** and **Game Server** wherever you want, as long as you know the IP of the machine running the **Game Server** and establish a known port.

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
