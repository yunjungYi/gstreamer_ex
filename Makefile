#GLS_DEV_HOME=/home/sooeun/Dev/GiGA_Streamer
GLS_LIBRARIES="/home/yjlee/lib/gstreamer-sdk/lib"
GLS_LIBRARIES64="/home/yjlee/lib/gstreamer-sdk/lib64"

#CFLAGS ="-g -O0"
CFLAGS = -std=c++11 -Wall -Wextra #-Wno-unused-parameter -Wno-unknown-pragmas -Wno-unused-function
CFLAGS += $(shell pkg-config --cflags gstreamer-1.0 gstreamer-rtsp-server-1.0 gstreamer-rtsp-1.0) -fpermissive 
CXXFLAGS="-g -O0"
LDFLAGS = $(shell pkg-config --libs gstreamer-1.0 gstreamer-rtsp-server-1.0 gstreamer-rtsp-1.0 gstreamer-plugins-bad-1.0) -L$(GLS_LIBRARIES) -lssl -lcrypto
TARGET = rtspd
SRC = $(wildcard rtspserver.cpp)
#SRC = $(wildcard ./sample/ex2.cpp)
OBJ = $(SRC:%.cpp=%.o)
all: $(TARGET)
CPP=g++

$(TARGET): $(OBJ)
	$(CPP) -Wl,-rpath,/home/yjlee/lib/gstreamer-sdk/lib -o $@ $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CPP) -c $< -o $@ $(CFLAGS) -I.

clean:
	rm -f *.o streamer
