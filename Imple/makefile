all: master server reader watchdog master_chat test_process_chat watchdog_chat writer
clean-logs:
	rm build/log/reader_0/* build/log/reader_1/* build/log/server/* 
clean:
	rm build/master build/server build/reader build/writer build/watchdog build/master_chat build/test_process_chat build/watchdog_chat
master:
	gcc master.c -o build/master
server:
	gcc server.c -o build/server
reader:
	gcc reader.c -o build/reader
watchdog:
	gcc watchdog.c -o build/watchdog
master_chat:
	gcc master_chat.c -o build/master_chat
test_process_chat:
	gcc test_process_chat.c -o build/test_process_chat
watchdog_chat:
	gcc watchdog_chat.c -o build/watchdog_chat
writer:
	gcc writer.c -o build/writer