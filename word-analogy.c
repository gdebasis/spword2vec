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

const long long N = 40;                  // number of closest words that will be shown
const int MAX_PATH_LEN = 1000;
const int max_w = 50;              // max length of vocabulary entries
int vecmode = 0;  // default bin mode... set to 1 for vec mode

static int isVecFile(char* fname) {
	int l = strlen(fname);
	return
		*(fname+l-1)=='c' && 
		*(fname+l-3)=='v';
}

int main(int argc, char **argv) {
  FILE *f;
  char st1[max_w];
  char bestw[N][max_w];
  char file_name[MAX_PATH_LEN], st[100][max_w]; 
  float dist, len, bestd[N], vec[max_w];
  long long words, size, a, b, c, d, cn, bi[100];
  float *M;
  char *vocab; 
  char *gt;
  int pAt1 = 0;
  float mrr = 0;
  int nRcds;
 
  if (argc < 2) {
    printf("Usage: ./word-analogy <BIN/VEC FILE>");
    return 0;
  }
  strncpy(file_name, argv[1], MAX_PATH_LEN);

	vecmode = isVecFile(file_name);

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
  
  for (b=0; b < words; b++) {
    a = 0;
    while (1) {
      vocab[b * max_w + a] = fgetc(f);
      if (feof(f) || (vocab[b * max_w + a] == ' ' || vocab[b * max_w + a] == '\t')) break;
      if ((a < max_w) && (vocab[b * max_w + a] != '\n')) a++;
    }
    vocab[b * max_w + a] = 0;

    for (a = 0; a < size; a++) !vecmode?
				fread(&M[a + b * size], sizeof(float), 1, f):
				fscanf(f, "%f", &M[a + b * size]);

    len = 0;
    for (a = 0; a < size; a++) len += M[a + b * size] * M[a + b * size];
    len = sqrt(len);
    for (a = 0; a < size; a++) M[a + b * size] /= len;

    //if (b%5000==0)
    //  printf("Loaded vector for %lld words\n", b);
  }
  fclose(f);

  nRcds = 0;
  while (1) {
    for (a = 0; a < N; a++) bestd[a] = 0;
    for (a = 0; a < N; a++) bestw[a][0] = 0;

    //if (nRcds%100==0) printf("Partial eval metrics after %d iterations: MRR=%.4f, P@1=%.4f\n", nRcds, mrr/(float)nRcds, pAt1/(float)nRcds);
    //printf("Enter three words (EXIT to break): ");
    a = 0;
    while (1) {
      st1[a] = fgetc(stdin);
      if ((st1[a] == '\n') || (a >= max_w - 1)) {
        st1[a] = 0;
        break;
      }
      a++;
    }
    if (!strcmp(st1, "EXIT")) break;
    cn = 0;
    b = 0;
    c = 0;
    while (1) {
      st[cn][b] = st1[c];
      b++;
      c++;
      st[cn][b] = 0;
      if (st1[c] == 0) break;
      if (st1[c] == ' ') {
        cn++;
        b = 0;
        c++;
      }
    }
    cn++;
    if (cn < 3) {
      printf("Only %lld words were entered.. three words are needed at the input to perform the calculation\n", cn);
      continue;
    }
    for (a = 0; a < cn; a++) {
      for (b = 0; b < words; b++) if (!strcmp(&vocab[b * max_w], st[a])) break;
      if (b == words) b = 0;
      bi[a] = b;
      printf("\nWord: %s  Position in vocabulary: %lld\n", st[a], bi[a]);
      if (b == 0) {
        printf("Out of dictionary word!\n");
        break;
      }
    }
    if (b == 0) continue;
    printf("\n                                              Word              Similarity\n------------------------------------------------------------------------\n");
    for (a = 0; a < size; a++) vec[a] = M[a + bi[1] * size] - M[a + bi[0] * size] + M[a + bi[2] * size];
    len = 0;
    for (a = 0; a < size; a++) len += vec[a] * vec[a];
    len = sqrt(len);
    for (a = 0; a < size; a++) vec[a] /= len;
    for (a = 0; a < N; a++) bestd[a] = 0;
    for (a = 0; a < N; a++) bestw[a][0] = 0;
    for (c = 0; c < words; c++) {
      if (c == bi[0]) continue;
      if (c == bi[1]) continue;
      if (c == bi[2]) continue;
      a = 0;
      for (b = 0; b < 3; b++) if (bi[b] == c) a = 1;
      if (a == 1) continue;
      dist = 0;
      for (a = 0; a < size; a++) dist += vec[a] * M[a + c * size];

      for (a = 0; a < N; a++) {
        if (dist > bestd[a]) {
          for (d = N - 1; d > a; d--) {
            bestd[d] = bestd[d - 1];
            strcpy(bestw[d], bestw[d - 1]);
          }
          bestd[a] = dist;
          strcpy(bestw[a], &vocab[c * max_w]);
          break;
        }
      }
    }
    for (a = 0; a < N; a++) printf("%50s\t\t%f\n", bestw[a], bestd[a]);

    // compute the MRR
    // ground-truth is the last word
    gt = &vocab[bi[3]*max_w];
    for (a = 0; a < N; a++) { 
        if (!strcmp(bestw[a], gt)) {
          if (a==0)
            pAt1++;
          mrr += 1/(float)(a+1);
        } 
    }
    nRcds++;
  }
  printf("#Instances = %d, MRR = %.4f, P@1 = %.4f\n", nRcds, mrr/(float)nRcds, pAt1/(float)nRcds);

  return 0;
}
