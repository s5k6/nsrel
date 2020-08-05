vpcc = gcc -std=c99 -g -Wall -Wextra -Wpedantic -Wbad-function-cast \
	-Wconversion -Wwrite-strings -Wstrict-prototypes -Wshadow

.PHONY : all

all : nsrel

nsrel : nsrel.c nsrel.help

%.help : %.txt
	sed -E 's/^/"/;s/$$/\\n"/' $< > $@

% : %.c
	$(vpcc) -o $@ $<
