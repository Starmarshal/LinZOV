BUILD 		= ./build
ROOT		= /usr/
BIN		= $(ROOT)/bin
SBIN		= $(ROOT)/sbin
SRC		= ./src

NAME		= archiver
PROG		:= $(BUILD)/$(NAME)
FOR_CC		:= $(shell find -wholename '$(SRC)/*.c')
CFLAGS		= -O2 -pedantic -Wall -Wextra -std=gnu99 -fomit-frame-pointer -fstack-protector-strong -Werror=format-security -o

CC 	 	= gcc

all: 		build
files: 		$(BIN) $(SBIN)

build: 		$(FOR_CC)
	@if [ ! -d $(BUILD) ]; then \
		echo "Creating local build"; \
		mkdir build; fi
	@echo "Compiling in progress"
	$(CC) $(FOR_CC) $(CFLAGS) $(PROG)

help:
	@echo "make to build into ./build"
	@echo "make instal as root to install ZOV"

intstall:
	@echo "Link archiver as zov"
	ln -s $(PWD)/archiver /usr/bin/zov

.PHONY: clean uninstall

clean:
	rm -rf build

uninstall:
	rm -rf /usr/bin/zov
