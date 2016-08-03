# 2016 Damien Firmenich

cc=g++
target=facesmoothing
halide=../Halide
libs=-lHalide -lpthread -ldl `libpng-config --cflags --ldflags`
includes=-I $(halide)/include -I $(halide)/tools -L $(halide)/bin

all: $(target)

$(target): facesmoothing.cpp
		$(cc) $^ $(includes) $(libs) -std=c++11 -o $@

example: facesmoothing
		./$(target) images/vis.png images/nir.png smoothed.png

clean:
		rm $(target)
