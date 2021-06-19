.PHONY: video

video: ../../../build/video.o

../../../build/video.o: video.h video.cpp shaders/*
	cd shaders && make
	g++ -Wall -O3 -std=c++17 -c video.cpp -o ../../../build/video.o -g
