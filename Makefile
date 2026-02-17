CXX = g++
CFLAGS = -std=c++20 -O2 -Iheaders $(shell sdl2-config --cflags)
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi $(shell sdl2-config --libs)

SRCS = $(wildcard **/*.cpp) $(wildcard *.cpp)

OBJS = $(SRCS:.cpp=.o)

TARGET = Engine


# Insert shader offline compilation here
define COMPILE_SHADERS
glslc Shaders/Samples/vertex.vert -o Shaders/Samples/vertex.vert.spv
glslc Shaders/Samples/fragment.frag -o Shaders/Samples/fragment.frag.spv
endef

# Default target
all: $(TARGET)


$(TARGET): $(OBJS)
	$(CXX) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@


test: $(TARGET)
	$(COMPILE_SHADERS)
	./$(TARGET) Engine 1280 720

run: CFLAGS += -DNDEBUG
run: $(TARGET)
	$(COMPILE_SHADERS)
	./$(TARGET) Engine 1280 720

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean test run