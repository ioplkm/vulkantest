CC = clang
CFLAGS = -std=c99 -O3 -Wall -Wextra -Wpedantic
LDFLAGS = -lglfw -lvulkan# -ldl -lpthread 
all: vulkantest shader
vulkantest: main.c physicalDevice.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o vulkantest main.c physicalDevice.c -DDEBUG
shader: shaders/shader.vert shaders/shader.frag
	glslc shaders/shader.vert -o shaders/vert.spv
	glslc shaders/shader.frag -o shaders/frag.spv
test: vulkantest
	./vulkantest
clean:
	rm vulkantest
	rm shaders/vert.spv
	rm shaders/frag.spv
