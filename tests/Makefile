#LD_VULKAN_FLAGS=-lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi
LD_VULKAN_FLAGS=-lglfw -lvulkan -ldl -lpthread -lX11 -lXrandr

.PHONY: tests %_test

tests: connection_test

connection_test: connection_test.cpp
	g++ -Wall -c ../src/common/connection/connection.cpp\
		-o ../build/connection.o
	g++ -Wall -fopenmp -o ../build/$@ $< ../build/connection.o -lgtest
	../build/$@

video_test: video_test.cpp
	cd ../src/client/video && make
	g++ -Wall -O3 -std=c++17 -fopenmp -o ../build/$@ $< ../build/video.o $(LD_VULKAN_FLAGS) -g
	../build/$@
#	valgrind --track-origins=yes --leak-check=full ../build/$@
