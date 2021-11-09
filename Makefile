GLS_DEV_HOME=/home/sooeun/Dev/GiGA_Streamer
GLS_SDK_PATH=$GLS_DEV_HOME/devenv
GLS_LIBRARIES="$GLS_SDK_PATH/lib"
GLS_LIBRARIES64="$GLS_SDK_PATH/lib64"

CFLAGS = -g -std=c++11 -Wall -Wextra #-Wno-unused-parameter -Wno-unknown-pragmas -Wno-unused-function
CFLAGS += $(shell pkg-config --cflags gstreamer-1.0 gstreamer-rtsp-server-1.0 gstreamer-rtsp-1.0) -fpermissive 
LDFLAGS = $(shell pkg-config --libs gstreamer-1.0 gstreamer-rtsp-server-1.0 gstreamer-rtsp-1.0 gstreamer-plugins-bad-1.0) -L$(GLS_LIBRARIES) -lssl -lcrypto
TARGET = streamer
SRC = $(wildcard *.cpp)
OBJ = $(SRC:%.cpp=%.o)
all: $(TARGET)
CPP=g++

$(TARGET): $(OBJ)
	$(CPP) -Wl,-rpath,/home/sooeun/Dev/gstreamer-sdk/lib -o $@ $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CPP) -c $< -o $@ $(CFLAGS) -I.

clean:
	rm -f *.o streamer
