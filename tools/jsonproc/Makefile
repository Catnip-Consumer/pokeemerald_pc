CXX ?= g++

CXXFLAGS := -Wall -std=c++17 -O2

INCLUDES := -I . -I ../../extern/json/include

SRCS := jsonproc.cpp

HEADERS := jsonproc.h inja.hpp

ifeq ($(OS),Windows_NT)
EXE := .exe
else
EXE :=
endif

.PHONY: all clean

all: jsonproc$(EXE)
	@:

jsonproc$(EXE): $(SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS) -o $@ $(LDFLAGS)

clean:
	$(RM) jsonproc jsonproc.exe
