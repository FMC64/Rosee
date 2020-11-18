CXX = clang++
CXXFLAGS = -Wall -Wextra -std=c++20

#RELEASE = true
#DEBUG = true
#SANITIZE = true

ifdef SANITIZE
DEBUG = true
CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
LD_LIBS += -lasan -lunsan
endif
ifdef DEBUG
CXXFLAGS += -g
endif
ifdef RELEASE
CXXFLAGS += -O3
endif

SRC = $(wildcard src/*.cpp) $(wildcard src/*/*.cpp)
OBJ = $(SRC:.cpp=.o)

TARGET = rosee

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)
