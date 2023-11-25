all: master server drone input world obstacles targets watchdog 
	
clean-logs:
	rm build/log/watchdog/* 
clean:
	rm build/master build/simple_process build/watchdog build/server build/drone build/input build/world build/obstacles build/targets
master:
	gcc master.c -o build/master
server:
	gcc server.c -o build/server
drone:
	gcc drone.c -o build/drone -lncurses -lm
input:
	gcc input.c -o build/input -lncurses
world:	
	gcc world.c -o build/world -lncurses
obstacles:
	gcc obstacles.c -o build/obstacles -lncurses
targets:
	gcc targets.c -o build/targets -lncurses
watchdog:
	gcc watchdog.c -o build/watchdog -lncurses
