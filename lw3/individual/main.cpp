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

typedef struct FilterProps {
  int M[9];
  int div;
  int offset;
} FilterProps;

enum Filters {
  GAUSS     = 0,
  EMBOS     = 1,
  SHAPE     = 2,
  LAPLASIAN = 3,
  RELIEF135 = 4,
  RELIEF90  = 5,
};

FilterProps dispatch_filter(int filter_id) {
  FilterProps props;

  switch (filter_id) {
    case GAUSS: {
      int M[9] = {0, 1, 0, 1, 4, 1, 0, 1, 0};
      copy(M, M + 9, props.M);
      props.div = 8;
      props.offset = 0;
      break;
    }
    
    case EMBOS: {
      int M[9] = {-2, -1, 0, -1, 1, 1, 0, 1, 2};
      copy(M, M + 9, props.M);
      props.div = 4;
      props.offset = 0;
      break;
    }

    case SHAPE: {
      int M[9] = {0, -1, 0, -1, 9, -1, 0, -1, 0};
      copy(M, M + 9, props.M);
      props.div = 5;
      props.offset = 128;
      break;
    }

    case LAPLASIAN: {
      int M[9] = {-1, -1, -1, -1, 8, -1, -1, -1, -1};
      copy(M, M + 9, props.M);
      props.div = 1;
      props.offset = 128;
      break;
    }

    case RELIEF135: {
      int M[9] = {1, 0, 0, 0, 0, 0, 0, 0, -1};
      copy(M, M + 9, props.M);
      props.div = 1;
      props.offset = 128;
      break;
    }

    case RELIEF90: {
      int M[9] = {0, 1, 0, 0, 0, 0, 0, -1, 0};
      copy(M, M + 9, props.M);
      props.div = 2;
      props.offset = 128;
      break;
    }

    default: {
      props.div = -1;
    }
  }
  return props;
}


void apply_filter(FilterProps p) {

  int id, size;
  int ARRAY_SIZE = width * height;

  #pragma omp parallel private(size, id)
  {
    id = omp_get_thread_num();
    size = omp_get_num_threads();

    int integer_part = width / size;
    int remainder = width % size;
    int a_local_size = integer_part + ((id < remainder) ? 1 : 0);

    int start = integer_part * id + ((id < remainder) ? id : remainder) + (id == 0 ? 1 : 0);

    int end = start + a_local_size + (id == size - 1 ? -1 : 0);

    for (int i = start; i < end; i++) {
      for (int j = 1; j < height - 1; j++) {
        double f = 0;
        for (int fi = 0; fi < 9; fi++) {
          f += image[(i + (fi / 3))*width + (j + (fi % 3))] * p.M[fi];
        }
        result_image[i*width + j] = int(f) / p.div + p.offset;
      }
    }
  }
}

int main(int argc, char* argv[]) {

  if (argc != 2) {
    cout << "Не указан фильтр" << endl;
    return -2;
  }

  FilterProps props = dispatch_filter(atoi(argv[1]));
  if (props.div == -1) {
    cout << "Возможные фильтры:\n" << endl;
    cout << "GAUSS     = 0" << endl;
    cout << "EMBOS     = 1" << endl;
    cout << "SHAPE     = 2" << endl;
    cout << "LAPLASIAN = 3" << endl;
    cout << "RELIEF135 = 4" << endl;
    cout << "RELIEF90  = 5" << endl;
    return -1;

  }

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  readBMP("cat1.bmp");
  saveBMP("gray.bmp");

  apply_filter(props);
  saveBMP("filtered.bmp");

  clock_gettime(CLOCK_MONOTONIC, &end);
  cout << "time: " << timeDiff(start, end) << endl;
}