CXX = clang++
CXXFLAGS_BASE = -std=c++20
CXXFLAGS = -Wall -Wextra $(CXXFLAGS_BASE)

WINDOWS = true
#LINUX = true

RELEASE = true
#DEBUG = true
#SANITIZE = true

ifdef SANITIZE
DEBUG = true
CXXFLAGS_BASE += -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined
#LD_LIBS += -shared-libsan
endif
ifdef DEBUG
CXXFLAGS_BASE += -g
endif
ifdef RELEASE
CXXFLAGS_BASE += -O3
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
OBJ_DEP = $(ROSEED)/Vma.o
SRC = $(SRCD)/main.cpp $(ROSEE_SRC)
OBJ = $(SRC:.cpp=.o) $(OBJ_DEP)

%.vert.spv: %.vert
	glslangValidator $< -V -o $@

%.frag.spv: %.frag
	glslangValidator $< -V -o $@

SHAD = sha
SHA = $(SHAD)/particle.frag $(SHAD)/particle.vert
SHA_VERT = $(SHA:.vert=.vert.spv)
SHA_FRAG = $(SHA:.frag=.frag.spv)

TARGET = rosee

all: $(TARGET) $(SHA_VERT) $(SHA_FRAG)

$(TARGET): $(OBJ) $(OBJ_DEP)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(TARGET) $(LD_LIBS)

$(ROSEED)/Vma.o:
	$(CXX) $(CXXFLAGS_BASE) -Wno-nullability-completeness $(ROSEED)/Vma.cpp -c -o $(ROSEED)/Vma.o

clean:
	rm -f $(OBJ) $(TARGET)
