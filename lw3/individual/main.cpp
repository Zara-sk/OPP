#include <omp.h>
#include <iostream>
#include <cstdio>
#include <cstring>

#include <time.h>

int width, height;
unsigned char* image;
unsigned char* result_image;

using namespace std;

void readBMP(char* filename) {
  unsigned char* img;
  FILE* f = fopen(filename, "rb");
  unsigned char info[54];

  // read the 54-byte header
  fread(info, sizeof(unsigned char), 54, f); 

  // extract image height and width from header
  width = *(int*)&info[18];
  height = *(int*)&info[22];
  int row_padded = (width*3 + 3) & (~3);
  // allocate 3 bytes per pixel
  int size = 3 * width * height;
  img = new unsigned char[size];
  image = new unsigned char[width * height];
  result_image = new unsigned char[width * height];

  // read the rest of the data at once
  fread(img, sizeof(unsigned char), size, f); 
  fclose(f);

  int i, j;
  for(i = 0, j = 0; i < size; i += 3, j += 1)
  {
    // Считаем интенсивность
    image[j] = (int)(0.2126*(int)(img[(i + 84 % width)+0])) +\
               (int)(0.7152*(int)(img[(i + 84 % width)+1])) +\
               (int)(0.0722*(int)(img[(i + 84 % width)+2]));
    result_image[j] = image[j];
  }
}

void saveBMP(char* filename) {
  FILE *f;
  int w = width, h = height;

  unsigned char *img = NULL;
  int filesize = 54 + 3*w*h;

  img = (unsigned char *)malloc(3*w*h);
  memset(img,0,3*w*h);

  int x, y, r;
  #pragma omp parallel for private (x, y, r)
  for(int i=0; i<w; i++)
  {
    for(int j=0; j<h; j++)
    {
      x=i; y=(h-1)-j;
      r = result_image[i + width * j];

      if (r > 255) r=255;

      img[(x+y*w)*3+2] = (unsigned char)(r);
      img[(x+y*w)*3+1] = (unsigned char)(r);
      img[(x+y*w)*3+0] = (unsigned char)(r);
    }
  }

  unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
  unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
  unsigned char bmppad[3] = {0,0,0};

  bmpfileheader[ 2] = (unsigned char)(filesize    );
  bmpfileheader[ 3] = (unsigned char)(filesize>> 8);
  bmpfileheader[ 4] = (unsigned char)(filesize>>16);
  bmpfileheader[ 5] = (unsigned char)(filesize>>24);

  bmpinfoheader[ 4] = (unsigned char)(       w    );
  bmpinfoheader[ 5] = (unsigned char)(       w>> 8);
  bmpinfoheader[ 6] = (unsigned char)(       w>>16);
  bmpinfoheader[ 7] = (unsigned char)(       w>>24);
  bmpinfoheader[ 8] = (unsigned char)(       h    );
  bmpinfoheader[ 9] = (unsigned char)(       h>> 8);
  bmpinfoheader[10] = (unsigned char)(       h>>16);
  bmpinfoheader[11] = (unsigned char)(       h>>24);

  f = fopen(filename, "wb");
  fwrite(bmpfileheader, 1, 14, f);
  fwrite(bmpinfoheader, 1, 40, f);
  for(int i = 0; i < h; i++)
  {
    fwrite(img + (w * (h-i-1) *3), 3, w, f);
    fwrite(bmppad, 1, (4 - (w*3) % 4) % 4, f);
  }

  free(img);
  fclose(f);
}

static double timeDiff(timespec start, timespec stop) {
  return 1.0 * (stop.tv_sec - start.tv_sec) +
         1.0 * (stop.tv_nsec - start.tv_nsec) / 1.0e+9;
}

void relief135_filter() {
  int M[9] = {1, 0, 0,0,  0, 0, 0, 0, -1};
  #pragma omp parallel for 
  for (int i = 1; i < width - 1; i++) {
    for (int j = 1; j < height - 1; j++) {
      double f = 0;
      for (int fi = 0; fi < 9; fi++) {
        f += image[(i + (fi / 3))*width + (j + (fi % 3))] * M[fi];
      }
      result_image[i*width + j] = int(f) / 1  + 128;
    }
  }
}

void relief90_filter() {
  int M[9] = {0, 1, 0, 0, 0, 0, 0, -1, 0};
  #pragma omp parallel for 
  for (int i = 1; i < width - 1; i++) {
    for (int j = 1; j < height - 1; j++) {
      double f = 0;
      for (int fi = 0; fi < 9; fi++) {
        f += image[(i + (fi / 3))*width + (j + (fi % 3))] * M[fi];
      }
      result_image[i*width + j] = int(f) / 2 + 128;
    }
  }
}

void laplasian_filter() {
  int M[9] = {-1, -1, -1, -1, 8, -1, -1, -1, -1};
  #pragma omp parallel for 
  for (int i = 1; i < width - 1; i++) {
    for (int j = 1; j < height - 1; j++) {
      double f = 0;
      for (int fi = 0; fi < 9; fi++) {
        f += image[(i + (fi / 3))*width + (j + (fi % 3))] * M[fi];
      }
      result_image[i*width + j] = int(f) / 1  + 208;
    }
  }
}
void shape_filter() {
  int M[9] = {0, -1, 0, -1, 9, -1, 0, -1, 0};
  #pragma omp parallel for 
  for (int i = 1; i < width - 1; i++) {
    for (int j = 1; j < height - 1; j++) {
      double f = 0;
      for (int fi = 0; fi < 9; fi++) {
        f += image[(i + (fi / 3))*width + (j + (fi % 3))] * M[fi];
      }
      result_image[i*width + j] = int(f) / 5 + 128 ;
    }
  }
}

void gauss_filter() {
  int M[9] = {0, 1, 0, 1, 4, 1, 0, 1, 0};
  #pragma omp parallel for 
  for (int i = 1; i < width - 1; i++) {
    for (int j = 1; j < height - 1; j++) {
      double f = 0;
      for (int fi = 0; fi < 9; fi++) {
        f += image[(i + (fi / 3))*width + (j + (fi % 3))] * M[fi];
      }
      result_image[i*width + j] = int(f) / 8 ;
    }
  }
}

void embos_filter() {
  int M[9] = {-2, -1, 0, -1, 1, 1, 0, 1, 2};
  #pragma omp parallel for 
  for (int i = 1; i < width - 1; i++) {
    for (int j = 1; j < height - 1; j++) {
      double f = 0;
      for (int fi = 0; fi < 9; fi++) {
        f += image[(i + (fi / 3))*width + (j + (fi % 3))] * M[fi];
      }
      result_image[i*width + j] = int(f) / 1 ;
    }
  }
}

int main() {  
  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  readBMP("cat1.bmp");
  saveBMP("gray.bmp");
  embos_filter();
  saveBMP("filtered.bmp");

  clock_gettime(CLOCK_MONOTONIC, &end);
  cout << "time: " << timeDiff(start, end) << endl;
}