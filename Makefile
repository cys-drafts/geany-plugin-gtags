all: gtags.so

gtags.so: plugin.c gtags.c gtags.h
	gcc -o $@ -g -fPIC `pkg-config --cflags geany` -shared `pkg-config --libs geany` $^

clean:
	rm -f gtags.so

.PHONY: clean all
