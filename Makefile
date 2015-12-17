#COMAKE2 edit-mode: -*- Makefile -*-
####################64Bit Mode####################
ifeq ($(shell uname -m),x86_64)
CC=gcc
CXX=g++
CXXFLAGS=-g \
  -pipe \
  -W \
  -Wall \
  -fPIC
CFLAGS=-g \
  -pipe \
  -W \
  -Wall \
  -fPIC
CPPFLAGS=-g \
  -O2 \
  -pipe \
  -W \
  -Wall \
  -fPIC \
  -Wno-deprecated \
  -D__const__=
#  -Werror 
INCPATH=-I. \
  -I./include \
  -I./ \
  -I./nginx/include/objs/ \
  -I./nginx/include/core/ \
  -I./nginx/include/event/ \
  -I./nginx/include/event/modules/ \
  -I./nginx/include/os/unix/ \
  -I./nginx/include/proc/

objs=ngx_nrpc_module.o \
	 log.o \
	 protocol.o \
	 server.o \
	 service_set.o

.PHONY:all
all:libnrpc.a UT
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mall[0m']"
	@echo "make all done"

.PHONY:clean
clean:
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mclean[0m']"
	rm -rf $(objs)
	rm -rf libnrpc.a
	make -C test clean

.PHONY:love
love:
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mlove[0m']"
	@echo "make love done"

libnrpc.a:$(objs)
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mlibnrpc.a[0m']"
	ar rcs libnrpc.a $(objs)

$(objs): %.o: %.cpp
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40m$@[0m']"
	$(CXX) -c $(INCPATH) $(CPPFLAGS) $(CXXFLAGS)  -o $@ $<


# UT
UT:
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mtest[0m']"
	make -C test all

endif #ifeq ($(shell uname -m),x86_64)


