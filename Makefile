CXX       ?= g++
CXXFLAGS  ?= -std=c++17 -O2 -Wall -Wextra
LDFLAGS   ?= -pthread

DEPS_DIR  := deps
CROW_DIR  := $(DEPS_DIR)/crow
ASIO_DIR  := $(DEPS_DIR)/asio

INCLUDES  := -I$(CROW_DIR)/include -I$(ASIO_DIR)/asio/include
TARGET    := server
SRC       := server.cpp

all: build

deps:
	@mkdir -p $(DEPS_DIR)
	@if [ ! -d "$(CROW_DIR)/.git" ]; then \
		git clone --depth 1 https://github.com/CrowCpp/Crow.git $(CROW_DIR); \
	fi
	@if [ ! -d "$(ASIO_DIR)/.git" ]; then \
		git clone --depth 1 https://github.com/chriskohlhoff/asio.git $(ASIO_DIR); \
	fi

build: $(TARGET)

$(TARGET): $(SRC)
	@if [ ! -d "$(CROW_DIR)/include" ] || [ ! -d "$(ASIO_DIR)/asio/include" ]; then \
		echo "Missing dependencies in $(DEPS_DIR)/. Run 'make deps'."; \
		exit 1; \
	fi
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

fclean: clean
	rm -rf $(DEPS_DIR)

.PHONY: all deps build run clean fclean
