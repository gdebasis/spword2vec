//  Copyright 2013 Google Inc. All Rights Reservede
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

const long long max_size = 100;         // max length of strings
const int LENGTH_NORMALIZE = 0;

int main(int argc, char **argv) {
  FILE *f, *ovec;
  char *word;
  float len; 
  char file_name[max_size], ofile_name[max_size];
  long long words, size, a, b;
  float *M;
  if (argc < 3) {
    printf("Usage: ./bin2vec <BINFILE> <VECFILE>");
    return 0;
  }
  strcpy(file_name, argv[1]);
  strcpy(ofile_name, argv[2]);
  f = fopen(file_name, "rb");
  if (f == NULL) {
    printf("Input file not found\n");
    return -1;
  }
  fscanf(f, "%lld", &words);
  fscanf(f, "%lld", &size);
 
  word = (char *)malloc(max_size * sizeof(char));
  M = (float *)malloc((long long)size * sizeof(float));
  if (M == NULL) {
    printf("Cannot allocate memory: %lld MB    %lld  %lld\n", (long long)words * size * sizeof(float) / 1048576, words, size);
    return -1;
  }

  ovec = fopen(ofile_name, "w");
  if (ovec == NULL) {
    printf("Couldn't open output file %s for writing\n", ofile_name);
    return -1;
  }
   
  fprintf(ovec, "%lld %lld\n", words, size);
  for (b = 0; b < words; b++) {
    a = 0;
    while (1) {
      word[a] = fgetc(f);
      if (feof(f) || (word[a]== ' ')) break;
      if ((a < max_size) && (word[a] != '\n')) a++;
    }
    word[a] = 0;
    for (a = 0; a < size; a++) fread(&M[a], sizeof(float), 1, f);

    if (LENGTH_NORMALIZE) {
      len = 0;
      for (a = 0; a < size; a++) len += M[a] * M[a];
      len = sqrt(len);
      for (a = 0; a < size; a++) M[a] /= len;
    }

    fprintf(ovec, "%s ", word);

    for (a = 0; a < size; a++) if (a==size-1) fprintf(ovec, "%.4f\n", M[a]); else fprintf(ovec, "%.4f ", M[a]);
  }
  fclose(f);
  fclose(ovec);

  return 0;
}
