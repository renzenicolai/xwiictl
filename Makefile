all: xwiictl

xwiictl: xwiictl.o
	g++ xwiictl.o -o xwiictl -lxwiimote -lX11 -lXtst

xwiictl.o:
	g++ -c xwiictl.cpp -lxwiimote -lX11 -lXtst

clean:
	rm -rf *.o xwiictl
