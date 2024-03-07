# Third assignment  

Project of Andrea Chiappe s4673275, Simone Lombardi s6119159

## Drone simulator  
In this simulator the drone is rappresented by a X inside a windows.
The drone has 8 degreeds of freedom in the plane, it could move up, down, right, left, and in all the four diagonal.
When the simlator is open, you will see two windows, one with the drone and the other with the keys on keyboard is possible to push.
Every keys correspond to a force apply at the drone. It will be expleined after.

### Installation

After downloading the repo with the command:

```
git clone https://github.com/SimoneLombardi/ARP_3.git
```

### Folder Structure

The downloaded folder is composed bu 5 folder inside: `bin`, `config`, `lib`, `log`, `src`, a file run.sh and a file README.md.
the first `bin` contain the executables of the programm. The `config` contain all the configuration file, including the parameters' file and the graphics. The `lib` contain the executaile of the library. in `log` there are the log file: `logfile.txt`, `logfile_sock.txt`, `logfile_wd.txt`. The folter `src` contains all the file .c of the project.

### Launch Program

After changed the correct repository is the time to run the program. In the folder willbe present 5 folders: `bin`, `config`, `lib`, `log`, `src`, a file run.sh and a file README.md. At this point in necessary to make the run.sh file executable. 
For make run.sh executable use this command on shell:

```
chmod +x run.sh
```

Now the file is executable and is possile to run the program using running run.sh on shell:

```
./run.sh
```

Now the programm will start runnng and 3 windows will open: server, input and rule_print

### Initialization 

After the opening of the three window the first operation is to focus on the rule_print one. In this window are explained all the rule usefull for playng with simulator. 
Here is necessary to wtite on the low bar two data: ip and port. 
Here we have two possibilities:
- If You want to play single player you have to insert inside as ip: localhost and as port 50000. Where 50000 is the port of the pc where you are listening on for target and obstacle.
- If you want to play with an other pc you have to insert here the ip of the otehr pc and the port where the other pc is listening you for send the messages. In this modality the pc must to be connected to the same network. In this case at the other pc is necessary to give information of our ip and our listenig port thet is the 50000.


### Use of Simulator

You can interact with the simulator by typing 8 different keys: D, S, F, E, C, W, R, X, V, (be sure you are typing the input in the "input" terminal). If you push one of these keys, the drone will move in one of the allowable directions corresponding to the pushed key. The idea is that when you push one of these keys, a step of force is applied to the drone, allowing it to move in the desired direction. Pushing the same key multiple times increases the force and, consequently, the speed. Each time you push the same key, the force increases by one step. To stop the drone, you can push the D key to remove all the force, or type the opposite keys compared to the drone's directions. When different keys are pushed, the drone moves in the resultant direction, which is the sum of all the applied forces.

The keys you can push are:
- **D:** Decrease one step of all the forces in the game
- **S:** Move the drone to the left
- **F:** Move the drone to the right
- **E:** Move the drone up
- **C:** Move the drone down
- **W:** Move the drone up-left diagonal
- **R:** Move the drone up-right diagonal
- **X:** Move the drone down-left diagonal
- **V:** Move the drone down-right diagonal


 
### Project Structure

The simulator, written in POSIX standard, consists of 5 processes working concurrently: `master.c`, `server.c`, `socket.c`, `drone.c`, `input.c`, `obstacle.c`, `targhet.c`, and `wd.c`.

- **`master.c`:** Creates all processes using fork/exec* sys calls. It creates the server and input processes for a graphical interface. This process also creates the necessary pipes for communication between processes. All the communication is done using pipes, the initialization pipes characterized by a numeric index fd_<progessive_number> are used to ensure that the Watchdog process is in possession of the correct pid of each process. Exception made for the WD before starting the normal execution each process sends it's pid back to the master process, and using the environment variable argc and argv the WD is able to recive all the correct pid.

  The primitives used by the master are:
  - fork() : creating new process
  - execvp() : using a process to launch an executable
  - pipe() : creating a pipe
  - close() : safely closing an unused file descriptor
  - fopen() : opening the log file
  - waitpid() : halting execution to recive status from a particular process
  

- **`input.c`:** Using the ncurses library the input process is able to manage the input created by the user and sends the result to the server process. The input is generated in char, the process manages the input to create a vector of double which represents the resultant force on the X and Y axes of the environment. After computing those values the process uses a pipe to send the information to the server, after this the process uses the input and the ncurses library to adequately inform the user of the situation.
 
  The primitives used by the input are:
  - pipe() : creating a pipe
  - fopen() : opening the log file
  - write() : sending data on a pipe
  - sigaction() : dealing with signals
  - kill() : send signals to the WD
  

- **`drone.c`:** This process is used to compute the dynamics of the drone, we used two pipe to implement a double direction communication with the server process. The drone uses a pipe controlled by a select to retrive the information about the total force that is applied to the drone in a particular moment, than using the formulas:

```math
V(t+dt) = V(t) + dt ( F + k V(t))
```
where
+ $V(t+dt)$ : Velocity of the drone in the next time istant
+ $V(t)$ : Actual drone velocity
+ $dt$ : delta time 
+ $F$ : resultant of all the external forces acting on the drone
+ $k$ : viscous friction coefficient
and
```math
P(t+dt) = P(dt) + dt V(t+dt)
```
where
+ $P(t+dt) : Position of the drone in the next time istant
+ $P(t) : Actual drone position

We were able to simulate the motion of the drone in the environment

The primitives used by the drone are:
  - fopen() : opening the log file
  - write() : sending data on a pipe
  - read() : readind data from a pipe
  - sigaction() : dealing with signals
  - select() : avoiding the blocking effect of the read() on the pipe
  - kill() : send signals to the WD

- **`obstacle.c`:** The obstacle process is in charge of generating the obstacle on a timer, we opted to simplify the communication needed by generating the random position as a double value between -0.5 and +0.5, by doing so we avoid the necessity to send the dimension of the map to the process every time it changes. The process uses a pipe to send an array of float value to the server.
  The primitives used by the obstacle are:
  - fopen() : opening the log file
  - write() : sending data on a pipe
  - sigaction() : dealing with signals
  - kill() : send signals to the WD

- **`target.c`:** The generation of the target follows the same logic of the obstacle, but, instead of generating the targhet on a timer the process generates one set of target for each game played.
  
   The primitives used by the targhet are:
  - fopen() : opening the log file
  - write() : sending data on a pipe
  - sigaction() : dealing with signals
  - kill() : send signals to the WD


- **`server.c`:** The server process is in charge of printing the map to the terminal and computing the repulsive force of the obstacle and limit of the work environment.
 The force of the obstacle are computed in the following way, depending on the distance from the drone:(to simplify the comparison the distance considered are all int values)
if the distance is higher of a constant R
```math
F_{obst} = 0 N
```
else if the distance is in between of r and R
```math
F_{obst} = (K |F_{inp}|) / (d)^2
```
where
+ $K$ : linear gain
+ $|F_{inp}|$ : valore assoluto della risultante delle forze in input
+ $d$ : distance of the drone from the obstacle

else if the distance is lower than r
```math
F_{obst} = K
```
this is done to avoid the $F_{obst}$ tend towards infinity as the distance from the obstacle goes to zero

The primitives used by the server are:
  - fopen() : opening the log file
  - write() : sending data on a pipe
  - sigaction() : dealing with signals
  - kill() : send signals to the WD


- **`socket.c`:** This process is responsable for handling the socket communication. This process start by implementing a socket serevr that waits for connection by some clients. After a fork() the child process implements a client that separates in two different process to handle the connection from a external target and obstacle process. This implementation is able to simultaneausly recive data and send data to another machine. 

The primitives used by the socket are:
  - bind() : nameing the file descriptor
  - socket() : obtaining the file descriptor for the soket
  - listen() : allow the server to accept connection
  - accept() : establish connection with a client 
  - connect() : connect a client to a server
  - close() : close the connection of the fd
  - fopen() : opening the log file
  - write() : sending data on a pipe
  - sigaction() : dealing with signals
  - kill() : send signals to the WD


- **`wd.c`:** Checks if all processes are running correctly. The watchdog works with signals, sending a signal to a different process every second. The process that receives the signal sends another signal back to the watchdog to inform that it is working. If the watchdog receives the signal back, everything is okay; otherwise, it means that the process wasn't working correctly, and it kills all the processes. At the start, the watchdog reads process ids from the pipes because opening them with konsole, the pid is not the same as returned from exec in the master process.

### Project architecture
![architettura_terzo_assignment](https://github.com/SimoneLombardi/ARP_3/assets/146358714/bbefeb45-f4ba-4e84-9aa9-f9b65ce27483)


### Example of use 

With the use of the process rule print the user is asked to insert the info (ip and port number) to establish the connection to the other machine.

A simple example of the use of the simulator is, as follows: If I push the F key on the keyboard, the drone, identified by an X in the window, will move to the right with a force of "one." If I push the F key again, the drone will move faster to the right of the window. To stop the drone, I can:
- Push the D key, which reset all the forces in the game. The drone will stop as an effect of the drag in the simulation.
- Push the S key, to increase the force in the opposite direction, pushing S the same number of times of F will have the same effect of pressing D, but if you press S one more time the drone will stop than procede in the S direction with force of "one".








