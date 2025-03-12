# SPDX-License-Identifier: GPL-2.0
# Makefile for LEDs tools

CFLAGS = -Wall -Wextra -g -I../../driver/include/uapi

all: uledmon
%: %.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) uledmon

.PHONY: all clean
