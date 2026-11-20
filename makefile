CFLAGS = -Wall -ansi -pedantic 
MAT_LIB = -lm
#HDRS_DIR   = hdrs
SRC_DIR = srcs
BUILD_DIR = build

SRCS = $(shell find $(SRC_DIR) -name '*.c')
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
TARGET = assembler

# .exe creation
assembler: $(OBJS)
	gcc $(CFLAGS) -o $@ $^ $(MAT_LIB)

# .o files-creation
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	gcc $(CFLAGS) -c $< -o $@ $(MAT_LIB)

clean:
	rm -rf $(BUILD_DIR) assembler *.ent *.ext *.ob *.am

test: assembler
	sh scripts/smoke-test.sh

.PHONY: clean test