objs = main.o svr_mgr.o
g = g++ -std=gnu++1z

start_up :  $(objs)
	$(g) -o start_up main.o svr_mgr.o -lpthread

main.o : main.cpp
	$(g) -c main.cpp
svr_mgr.o : svr_mgr.cpp
	$(g) -c svr_mgr.cpp
.PHONY: clean
clean :
	-rm  start_up $(objs)