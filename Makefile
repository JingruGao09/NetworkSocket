CFLAGS= #-pedantic -Werror -Wall #-O3 -fPIC  -lpthread

TARGETS=ringmaster player

all: $(TARGETS)
clean:
	rm -f $(TARGETS)

player: player.cpp potato.hpp
	g++ -o $@ $<

ringmaster: ringmaster.cpp potato.hpp
	g++ -o $@ $<
