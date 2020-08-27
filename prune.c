//  Copyright 2013 Google Inc. All Rights Reserved.
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

const int MAX_PATH_LEN = 1000;
const int max_w = 50;              // max length of vocabulary entries
int vecmode = 0;  // default bin mode... set to 1 for vec mode

static int isVecFile(char* fname) {
	int l = strlen(fname);
	return
		*(fname+l-1)=='c' && 
		*(fname+l-2)=='e' && 
		*(fname+l-3)=='v';
}

static int cmp_words(const void* e1, const void* e2) {
  return strcmp((char *) e1, (char *) e2);
}

int main(int argc, char **argv) {
  FILE *f, *fo;
  const int MAX_LIMITED_VOCAB = 100000;
  char file_name[MAX_PATH_LEN], limited_vocab_file_name[MAX_PATH_LEN], ofile_name[MAX_PATH_LEN];
  long long words, size, a, b; 
  float *M;
  char *vocab, limited_vocab[MAX_LIMITED_VOCAB][max_w];
  int limited_vocab_count = 0;
  int nRcds = 0;
  char *found;
 
  if (argc < 4) {
    printf("Usage: ./prune <BIN/VEC FILE> <limiting vocab list> <outfile>\n");
    return 0;
  }
  strncpy(file_name, argv[1], MAX_PATH_LEN);
  strncpy(limited_vocab_file_name, argv[2], MAX_PATH_LEN);
  strncpy(ofile_name, argv[3], MAX_PATH_LEN);

	vecmode = isVecFile(file_name);

  // Load the limited vocab (if provided)

  f = fopen(limited_vocab_file_name, "r");
  if (f == NULL) {
    printf("Input file %s not found\n", limited_vocab_file_name);
    return -1;
  }

  while (fgets(&limited_vocab[limited_vocab_count][0], max_w, f)!= NULL && limited_vocab_count < MAX_LIMITED_VOCAB) {
    for (a=0; a<max_w; a++) {
      if (limited_vocab[limited_vocab_count][a]=='\n') {
        limited_vocab[limited_vocab_count][a]=0;
        break;
      }
    }
    limited_vocab_count++;
  }

  qsort(limited_vocab, limited_vocab_count, max_w, cmp_words);
  fclose(f);

  f = fopen(file_name, "rb");
  if (f == NULL) {
    printf("Input file not found\n");
    return -1;
  }
  fscanf(f, "%lld", &words);
  fscanf(f, "%lld", &size);
  vocab = (char *)malloc((long long)words * max_w * sizeof(char));
  M = (float *)malloc((long long)words * (long long)size * sizeof(float));
  if (M == NULL) {
    printf("Cannot allocate memory: %lld MB    %lld  %lld\n", (long long)words * size * sizeof(float) / 1048576, words, size);
    return -1;
  }

  if (limited_vocab_count==0)
    limited_vocab_count = words;

  for (b=0, nRcds=0; b < limited_vocab_count && nRcds<words; nRcds++) {
    a = 0;
    while (1) {
      vocab[b * max_w + a] = fgetc(f);
      if (feof(f) || (vocab[b * max_w + a] == ' ' || vocab[b * max_w + a] == '\t')) break;
      if ((a < max_w) && (vocab[b * max_w + a] != '\n')) a++;
    }
    vocab[b * max_w + a] = 0;

    found = bsearch(&vocab[b * max_w], limited_vocab, limited_vocab_count, max_w, cmp_words);

    for (a = 0; a < size; a++) !vecmode?
				fread(&M[a + b * size], sizeof(float), 1, f):
				fscanf(f, "%f", &M[a + b * size]);

    if (!found) // not found in limited vocab
      continue;  // reuse this store (note we don't increment b)

    b++;
    if (b%1000==0)
        printf("Loaded vector for %lld words\n", b);
  }
  fclose(f);

  words = b;

  fo = fopen(ofile_name, "wb");
  if (!fo) {
    printf("Couldn't open file %s for writing pruned vocab\n", ofile_name);
    return 1;
  }

  fprintf(fo, "%lld %lld\n", words, size);
  for (b = 0; b < words; b++) {
    fprintf(fo, "%s ", &vocab[b*max_w]);
		for (a = 0; a < size; a++) fwrite(&M[b*size+a], sizeof(float), 1, fo);
    fprintf(fo, "\n");
  }

  fclose(fo);

  return 0;
}
