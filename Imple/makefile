all: main server reader writer master simple_process watchdog
clean-logs:
	rm build/log/reader_0/* build/log/reader_1/* build/log/server/* 
clean:
	rm build/main build/server build/reader build/writer build/master build/simple_process build/watchdog
main:
	gcc main.c -o build/main
server:
	gcc server.c -o build/server
reader:
	gcc reader.c -o build/reader
writer:
	gcc writer.c -o build/writer
master:
	gcc master.c -o build/master
simple_process:
	gcc simple_process.c -o build/simple_process
watchdog:
	gcc watchdog.c -o build/watchdog -lncurses
