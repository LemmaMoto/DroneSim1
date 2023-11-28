# Advanced and Robot Programming - Asssignment 1 #
## Authors: Bua Odetti Emanuele, Malatesta Federico ##

### Introduction ###
This project involves the development of a drone operation interactive simulator. The simulator is built as a window using the NCurses library. Here's a breakdown of key components and functionalities:
* `master`: initiates multiple child processes using fork and execvp to execute distinct programs (watchdog, server, drone, input, world) in a simulated environment, passing arguments via pipes and handling process termination while utilizing inter-process communication to simulate a drone operation system.
* `watchdog`: utilizing NCurses library establishes a watchdog system monitoring multiple processes' responsiveness by tracking and displaying elapsed time. It reads process-specific timestamps from log files, updates respective windows, sends signals at intervals, and terminates processes if timeouts occur, providing a comprehensive monitoring tool for system processes.
* `server`: creates a process to manage a simulated drone's movement by sharing memory segments (using shmget and shmat) for coordinates and attributes between processes, periodically updating data and communicating with a watchdog process via signals to ensure responsiveness, while reading and writing process IDs in PID files for inter-process communication. 
* `drone`: uses shared memory segments and pipes to simulate a drone's movement, implementing a physical model with forces applied through user commands read from a pipe, updating its position and velocity in real-time based on received commands and physical calculations while communicating with a watchdog process via signals for logging and coordination.
* `input`: using the NCurses library, creates a graphical user interface (UI) that represents a grid of squares. It interacts with a drone simulation process via pipes, allowing users to input commands through the UI, updating square counts and controlling the simulated drone's movements based on user input.
* `world`: initializes a shared memory segment and uses the NCurses library to create a simple graphical representation of a drone's movement by reading its position from the shared memory, displaying the drone's symbol on the terminal, and refreshing the display periodically based on the shared memory updates.

[Link to Schematics](https://github.com/LemmaMoto/DroneSim1/blob/main/Arp%20schematics.pdf)



### How to install and run ###
1. Open the terminal
2. Clone the repository:
<div>
  <button onclick="copyCode()"></button>
</div>
<pre><code id="codeBlock">git clone https://github.com/LemmaMoto/DroneSim1
</code></pre>
3. Go to the `Imple` directory 
<div>
  <button onclick="copyCode()"></button>
</div>
<pre><code id="codeBlock">cd Imple 
</code></pre>
4. Run code using 
<div>
  <button onclick="copyCode()"></button>
</div>
<pre><code id="codeBlock">./run.sh
</code></pre>

### How to manage drone's movements ###
the keys to be pressed in the input window used to move the drone and to make other actions are:
* `w`: move up-left
* `e`: move up
* `r`: move up-right
* `s`: move left
* `d`: stop all forces 
* `f`: move right
* `x`: move down-right
* `c`: move down
* `v`: move down-left
* `q`: terminate the input process
* `b`: stop the drone 

