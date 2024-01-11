# Advanced and Robot Programming - Assignment 2 #
## Authors: Bua Odetti Emanuele, Malatesta Federico ##

### Introduction ###
This project involves the development of a drone operation interactive simulator. The simulator is built as a window using the NCurses library. Here's a breakdown of key components and functionalities:
* `master`: creates several child processes, each representing a different component of a system (like server, drone, input, world, obstacles, targets). It establishes communication between these processes using Unix pipes. The master process also handles the creation and removal of a log file. After creating the child processes, it waits for all of them to terminate, printing their exit statuses. Finally, it closes all the pipes and returns the last non-zero exit status.
* `watchdog`: utilizing NCurses library establishes a watchdog system monitoring multiple processes' responsiveness by tracking and displaying elapsed time. It reads process-specific timestamps from log files, updates respective windows, sends signals at intervals, and terminates processes if timeouts occur, providing a comprehensive monitoring tool for system processes.
* `server`: establishes multiple processes for distinct functionalities (watchdog, server, drone, input, world, obstacles, targets) utilizing inter-process communication via pipes. These processes handle shared data structures and continuously communicate through pipes to simulate updates in a loop. 
* `drone`: implement a simulation involving a drone navigating through obstacles and targets in a two-dimensional space. It uses forces like repulsion from obstacles and attraction towards targets to calculate the drone's movement. The program communicates through pipes with other processes and includes signal handling for synchronization purposes.
* `input`: using the NCurses library, creates a graphical user interface (UI) that represents a grid of squares. It interacts with a drone simulation process via pipes, allowing users to input commands through the UI, updating square counts and controlling the simulated drone's movements based on user input.
* `world`: reads the world state from a pipe, updates the screen dimensions, and writes the updated screen dimensions back to another pipe. It uses the ncurses library to display the drone, obstacles, and targets in a terminal window.
* `obstacles`: nitializes the process, reads parameters from a file, and sets up signal handling for communication with a watchdog process. The main loop reads the world state from a pipe, generates random obstacles within the world, and writes the updated obstacle data back to another pipe. The process also logs time updates to a file whenever it receives a signal from the watchdog process.
* `targets`: manage multiple targets handling their generation, activation, and interaction with a drone within the world process using inter-process communication through pipes and file I/O for configuration parameters. It continuously updates the state of targets based on a predefined refresh time and drone interaction.

[Link to Schematics](https://github.com/LemmaMoto/DroneSim1/blob/assignment2/ARP_schematics.pdf)

[Link to List of components](https://github.com/LemmaMoto/DroneSim1/blob/assignment2/list_of_components.txt)

[Link to Primitives](https://github.com/LemmaMoto/DroneSim1/blob/assignment2/list_syscalls.txt)



### How to install and run ###
1. Open the terminal
2. Clone the repository:
<div>
  <button onclick="copyCode()"></button>
</div>
<pre><code id="codeBlock">git clone https://github.com/LemmaMoto/DroneSim1
</code></pre>
3. Go to the Imple directory 
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
* `a`: respawn drone in the initial position

