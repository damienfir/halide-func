# 2016 Damien Firmenich

cc=g++
target=face_smoothing
halide=../Halide
libs=-lHalide -lpthread -ldl `libpng-config --cflags --ldflags`
includes=-I $(halide)/include -I $(halide)/tools -L $(halide)/bin

all: $(target)

$(target): src/face_smoothing.cpp
		$(cc) $^ $(includes) $(libs) -std=c++11 -o $@

example: $(target)
		LD_LIBRARY_PATH=$(halide)/bin ./$(target) images/vis.png images/nir.png smoothed.png

clean:
		rm $(target)
