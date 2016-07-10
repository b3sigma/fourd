CC        := g++
LD        := g++

BIN_NAME  := fourd
MODULES   := common app
SRC_DIR   := $(MODULES)
BUILD_DIR := $(addprefix build/,$(MODULES))

SRC       := $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.cpp))
OBJ       := $(patsubst %.cpp,build/%.o,$(SRC))
INCLUDES  := $(addprefix -I,$(SRC_DIR))

COMPILE_FLAGS = -std=c++11 -Wall -Wextra -g -Wno-unused-local-typedefs -Wno-unused-parameter -Wno-unknown-pragmas
LIBRARIES = -lX11 -lXi -lXmu -lglut -lGL -lGLU -lm -lCg -lCgGL
LINK_FLAGS = $(LIBRARIES)


vpath %.cpp $(SRC_DIR)

define make-goal
$1/%.o: %.cpp
	$(CC) $(COMPILE_FLAGS) $(INCLUDES) -c $$< -o $$@
endef

.PHONY: all checkdirs clean

all: checkdirs $(BIN_NAME)

$(BIN_NAME): $(OBJ)
	$(LD) $^ $(LINK_FLAGS) -o $@

checkdirs: $(BUILD_DIR)

$(BUILD_DIR):
	@mkdir -p $@

clean:
	@rm -rf $(BUILD_DIR)

$(foreach bdir,$(BUILD_DIR),$(eval $(call make-goal,$(bdir))))
