
X11_ROOT=/opt/X11/

ppmv: ppmv.c
	gcc -o ppmv -I${X11_ROOT}/include ppmv.c -L${X11_ROOT}/lib -lX11

clean:
	rm -f ppmv

.PHONY: clean

