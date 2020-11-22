CXX = clang++
CXXFLAGS = -Wall -Wextra -std=c++20

WINDOWS = true
#LINUX = true

RELEASE = true
#DEBUG = true
#SANITIZE = true

ifdef SANITIZE
DEBUG = true
CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
#LD_LIBS += -shared-libsan
endif
ifdef DEBUG
CXXFLAGS += -g
endif
ifdef RELEASE
CXXFLAGS += -O3
endif

ifdef WINDOWS
LD_LIBS += "$(shell cygpath --unix $(VULKAN_SDK))/Lib/vulkan-1.lib" -lglfw3
endif
ifdef LINUX
LD_LIBS += -lvulkan -lglfw
endif

SRCD = src
ROSEED = $(SRCD)/Rosee
ROSEE_SRC = $(ROSEED)/Brush.cpp $(ROSEED)/Cmp.cpp $(ROSEED)/Map.cpp $(ROSEED)/Renderer.cpp $(ROSEED)/Vk.cpp
SRC = $(SRCD)/main.cpp $(ROSEE_SRC)
OBJ = $(SRC:.cpp=.o)

%.vert.spv: %.vert
	glslangValidator $< -V -o $@

%.frag.spv: %.frag
	glslangValidator $< -V -o $@

SHAD = sha
SHA = $(SHAD)/particle.frag $(SHAD)/particle.vert
SHA_VERT = $(SHA:.vert=.vert.spv)
SHA_FRAG = $(SHA:.frag=.frag.spv)

TARGET = rosee

all: $(TARGET)

$(TARGET): $(OBJ) $(SHA_VERT) $(SHA_FRAG)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(TARGET) $(LD_LIBS)

clean:
	rm -f $(OBJ) $(TARGET)
