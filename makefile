all: findinpath.o
	gcc findinpath.o -g -o findinpath.exe
findinpath.o: findinpath.c
	gcc -c findinpath.c -g -o findinpath.o
release: findinpath.c
	gcc findinpath.c -o findinpath
.PHONY : install clean uninstall
install: release
	cp -f findinpath.exe ~/bin/findinpath
clean:
	rm -f findinpath.o findinpath.exe
uninstall:
	rm ~/bin/findinpath

