FLTKLIB=`fltk-config --ldflags`
FLTKINC=`fltk-config --cxxflags`
LIBS=$(FLTKLIB)
LIBS+=-lpng -ljpeg
INC=$(FLTKINC)
CC=g++

Convert:
	rm -f convert
	 $(CC) -Wall $(INC) convert.c -o convert $(LIBS)

clean:
	rm -f convert
