all: master server drone input world watchdog 
	
clean-logs:
	rm build/log/watchdog/* 
clean:
	rm build/master build/simple_process build/watchdog build/server build/drone build/input build/world
master:
	gcc master.c -o build/master
server:
	gcc server.c -o build/server
drone:
	gcc drone.c -o build/drone -lncurses
input:
	gcc input.c -o build/input -lncurses
world:	
	gcc world.c -o build/world -lncurses
watchdog:
	gcc watchdog.c -o build/watchdog -lncurses
