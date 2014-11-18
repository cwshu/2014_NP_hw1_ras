CC = gcc
CFLAGS =
CXX = g++
CXXFLAGS =

RAS_DIR = ~/ras
RAS_BIN_DIR = $(RAS_DIR)/bin
COMMANDS_DIR = commands
BIN_DIR = $(COMMANDS_DIR)/bin
RAS_DATA_DIR = ras_data

BUILD_SRC = $(wildcard $(COMMANDS_DIR)/*.cpp)
BUILD_SRC_NAME = $(notdir $(BUILD_SRC))
BUILD_EXES_NAME = $(patsubst %.cpp,%,$(BUILD_SRC_NAME))
BUILD_EXES = $(addprefix $(BIN_DIR)/,$(BUILD_EXES_NAME))
CP_EXES_NAME = ls cat
CP_EXES = $(addprefix $(BIN_DIR)/,$(CP_EXES_NAME))
CLIENT_TEST_PROGRAM = client

all: $(BUILD_EXES) $(CP_EXES) $(CLIENT_TEST_PROGRAM)
# make all command the binary into $(BIN_DIR)
# $(CP_EXES): copy from system binary
# $(BUILD_EXES): build from $(COMMANDS_DIR) directory

install: | $(RAS_BIN_DIR)
# copy all the binary into $(RAS_BIN_DIR)
	cp $(BIN_DIR)/* $(RAS_BIN_DIR)
	cp $(RAS_DATA_DIR)/* $(RAS_DIR)

clean:
	@rm -f $(BUILD_EXES) $(CP_EXES) $(CLIENT_TEST_PROGRAM)
	@rm -rf $(BIN_DIR)

uninstall:
	rm -rf $(RAS_DIR)

$(BUILD_EXES): $(BIN_DIR)/%: $(COMMANDS_DIR)/%.cpp
	$(CXX) -o $@ $(CXXFLAGS) $<

$(CP_EXES): $(BIN_DIR)/%: 
	cp `which $*` $(BIN_DIR)/

$(BUILD_EXES) $(CP_EXES): | $(BIN_DIR) 
# create $(BIN_DIR) before build/copy

$(RAS_DIR) $(RAS_BIN_DIR) $(BIN_DIR): 
# create directory
	mkdir -p $@

$(CLIENT_TEST_PROGRAM): client.c
	$(CC) -o $@ $(CFLAGS) $<

.PHONY: all install clean deinstall
