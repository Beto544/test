##LIB    = ./libggfonts.so


all: lab2 

lab2: xylab2.cpp
	g++ xylab2.cpp libggfonts.a -Wall -Wextra -olab2 -lX11 -lGL -lGLU -lm

clean:
	rm -f lab2

