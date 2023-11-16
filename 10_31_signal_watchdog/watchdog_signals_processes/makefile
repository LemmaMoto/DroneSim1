all: master simple_process watchdog
	
clean-logs:
	rm build/log/*
clean:
	rm build/master build/simple_process build/watchdog
master:
	gcc master.c -o build/master
simple_process:
	gcc simple_process.c -o build/simple_process
watchdog:
	gcc watchdog.c -o build/watchdog -lncurses
