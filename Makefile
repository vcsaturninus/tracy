CC:=gcc
CFLAGS:=-g -DTRACY_DEBUG_MODE
CPPFLAGS:=-I src/
TEST_DIR:=example
OUTDIR:=out/
SRC:=$(wildcard src/*.c $(TEST_DIR)/*.c)
VALGRIND_REPORT_NAME:=valgrind.log
OUTBIN:=test


all: clean build

build: clean
	@mkdir -p $(OUTDIR)
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(SRC) -o $(OUTDIR)/$(OUTBIN)

clean:
	@rm -rf $(OUTDIR) $(VALGRIND_REPORT_NAME)

grind: build
	@valgrind --leak-check=full --show-leak-kinds=all \
		--track-origins=yes --verbose \
		--log-file=$(VALGRIND_REPORT_NAME) $(OUTDIR)/$(OUTBIN)


	
