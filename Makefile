CC      := gcc
CFLAGS  := -O2 -std=c17 -Wall -Wextra -I./src -I./vendor/sha256 -I./vendor/xxhash -I.
LDFLAGS := -lm
TARGET  := ./compiled/mixer_test
OBJ_DIR := ./compiled/obj

# 全てのソースファイルをリストアップ
SRCS := $(shell find src -name "*.c") \
        $(shell find vendor -name "*.c" 2>/dev/null)

# .c を .o に置換しつつ、頭に OBJ_DIR をつける
# patsubst を使い、%.c を $(OBJ_DIR)/%.o に変換します
OBJS := $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS))

all: $(TARGET)

# リンク
$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# 汎用コンパイルルール
# $(OBJ_DIR)/ 以下の .o を作るには、対応するカレントディレクトリの .c を見る
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

PRNG_STDOUT := ./compiled/prng_stdout

$(PRNG_STDOUT): tools/prng_stdout.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) tools/prng_stdout.c -o $(PRNG_STDOUT) $(LDFLAGS)

prng_stdout: $(PRNG_STDOUT)

run: $(TARGET)
	$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(TARGET) $(PRNG_STDOUT)

.PHONY: all run prng_stdout clean