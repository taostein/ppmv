// 2014 Tao Stein 石涛
// Very fast rendering of PPMs on any system supporting X11

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// the PPM reading and writing part of the code

#define PPMREADBUFLEN 256
unsigned char* get_ppm(FILE *pf, unsigned int* width, unsigned int* height) {
  char buf[PPMREADBUFLEN], *t;
  unsigned char* img;
  unsigned int w, h, d;
  int r;

  if (pf == NULL) {
    return NULL;
  }
  t = fgets(buf, PPMREADBUFLEN, pf);
  // fail if the white space following "P6" is not '\n'
  if ((t == NULL) || (strncmp(buf, "P6\n", 3) != 0 )) {
    return NULL;
  }
  do {
    // Px formats can have # comments after first line
    t = fgets(buf, PPMREADBUFLEN, pf);
    if (t == NULL) {
      return NULL;
    }
  } while (strncmp(buf, "#", 1) == 0);

  // get width and height
  r = sscanf(buf, "%u %u", &w, &h);
  if (r < 2) {
    return NULL;
  }

  fprintf(stderr, "width = %u, height = %u\n", w, h);

  // get color width
  r = fscanf(pf, "%u", &d);
  if ((r < 1) || (d != 255)) {
    return NULL;
  }
  // skip one byte, should be whitespace
  fseek(pf, 1, SEEK_CUR);
  // 3 bytes per PPM color
  size_t img_len = w * h * 3;
  img = (unsigned char *) malloc(img_len);

  size_t rd = 0;
  while (1) {
    size_t rd1 = fread(img + rd, 1, img_len - rd, pf);
    rd += rd1;
    assert(!(rd > img_len));
    if (rd1 == 0) {
      break;
    }
  }
  fclose(pf);
  *width = w;
  *height = h;
  return img;
}

// A convenience function for outputting a PPM from a char buffer
void output_ppm(unsigned char * img,
    unsigned int width, unsigned int height, FILE* fp) {
  fprintf(stdout, "P6\n");
  fprintf(stdout, "%u %u\n", width, height);
  fprintf(stdout, "255\n");
  unsigned int i;
  for (i = 0; i < 3 * width * height; i++) {
    fputc(img[i], fp);
  }
  fclose(stdout);
}

// begin the X11 part of the code

Display *dis;
int screen;
Window win;
GC gc;

void init_x();
void close_x();
void redraw();

int main (int argc, char* argv[]) {
	XEvent event;
	KeySym key;
	char text[255];

  FILE* ppm_file = stdin;
  char* input_name = "stdin";
  unsigned int sleep_time = 0;
  if (argc > 1) {
    input_name = argv[1];
    if (strcmp(input_name, "-") == 0) {
      ppm_file = stdin;
    } else {
      ppm_file = fopen(input_name, "r");
    }
    if (!ppm_file) {
      fprintf(stderr, "could not open file %s\n", input_name);
      exit(1);
    }

    // is there a -s for sleep time
    if (argc == 4 && strcmp(argv[2], "-s") == 0) {
      sleep_time = atoi(argv[3]);
      fprintf(stderr, "sleep time = %u\n", sleep_time);
    } else if (argc > 2) {
      fprintf(stderr, "ppmv filename -s sleeptime\n");
      exit(1);
    }

  }

  unsigned int ppm_width, ppm_height;
  unsigned char* ppm = get_ppm(ppm_file, &ppm_width, &ppm_height);
  if (ppm == NULL) {
    fprintf(stderr, "failed to read ppm from %s\n", input_name);
    exit(1);
  }
  
	init_x(ppm_width, ppm_height, input_name);

  int start_time = 0;
  struct timeval tp;
  if (sleep_time > 0) {
    int t = gettimeofday(&tp, NULL);
    start_time = tp.tv_sec;
  }

	while (1) {

    if (sleep_time > 0) {
      int t = gettimeofday(&tp, NULL);
      int d = tp.tv_sec - start_time;
      if (tp.tv_sec - start_time > sleep_time) {
        close_x();
        exit(1);
      }
    }

    // draw the ppm to the window
    int x;
    for (x = 0; x < ppm_width * ppm_height * 3; x += 3) {
        unsigned char x1 = ppm[x];
        unsigned char x2 = ppm[x + 1];
        unsigned char x3 = ppm[x + 2];
        unsigned long color;
        if (ppm_file == stdin) {
          color = 0x0 | x2 << 16 | x3 << 8 | x1 << 0;
        } else {
          color = 0x0 | x1 << 16 | x2 << 8 | x3 << 0;
        }
        XSetForeground(dis, gc, color);
        int i = (x / 3) % ppm_width;
        int j = (x / 3) / ppm_width;
        XDrawPoint(dis, win, gc, i, j);
    }
  }
  return 1;
}

void init_x(unsigned int width, unsigned int height, char* input_name) {
  unsigned long black = 0x00, white = 0xffffff;

  dis = XOpenDisplay((char *)0);
  screen = DefaultScreen(dis);
  win = XCreateSimpleWindow(dis, DefaultRootWindow(dis), 0, 0,
    width, height, 0, black, white);
  XSetStandardProperties(dis, win, input_name, "", None, NULL, 0, NULL);
  // listen to KeyPressMask for 'q' quit
  XSelectInput(dis, win, ExposureMask|ButtonPressMask|KeyPressMask);
  gc = XCreateGC(dis, win, 0,0);
  XClearWindow(dis, win);
  XMapRaised(dis, win);
};

void close_x() {
	XFreeGC(dis, gc);
	XDestroyWindow(dis, win);
	XCloseDisplay(dis);	
	exit(1);
};

void redraw() {
	XClearWindow(dis, win);
};
