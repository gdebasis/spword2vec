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

// Compute distances between word pairs from a given text file of the following form
// <word1> \s <word2> \s <ref sim> (used only for evaluation)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAXWORDLEN 30
#define LINE_BUFF_SIZE 65536
#define MAX_VOCAB 500000

int vecmode = 0;  // default bin mode... set to 1 for vec mode

const int max_size = 2000;
const long long N = 10;                  // number of closest words that will be shown
const long long max_w = 50;              // max length of vocabulary entries

typedef struct WordPair {
	char word1[MAXWORDLEN];
	char word2[MAXWORDLEN];
	float sim;     // predicted
	float refsim;  // reference
	int pred_rank;
	int ref_rank;
}
WordPair;

char lineBuff[LINE_BUFF_SIZE];

int wordpair_compare_refsim(const void *a, const void *b) {
    return ((WordPair*)a)->refsim < ((WordPair*)b)->refsim? 1 : ((WordPair*)a)->refsim == ((WordPair*)b)->refsim? 0: -1 ;
}

int wordpair_compare_predsim(const void *a, const void *b) {
    return ((WordPair*)a)->sim < ((WordPair*)b)->sim? 1 : ((WordPair*)a)->sim == ((WordPair*)b)->sim? 0: -1 ;
}

int isVecFile(char* fname) {
	int l = strlen(fname);
	return
		*(fname+l-1)=='c' && 
		*(fname+l-2)=='e' && 
		*(fname+l-3)=='v';
}

int main(int argc, char **argv) {
  FILE *f;
  char st1[max_size];
  float dist, len;
  long long words, a, b; 
	int c, size, rank;
  float *M;
  char *vocab, *sep = "\n\t ", *token;
	int numLines = 0, i;
	const int MAX_LINES = 10000;
	float rho, prev_sim;
	int sum_rank_pred, sum_rank_ref;
	float mean_rank_pred, mean_rank_ref;
	float del_x, del_y, cov_xy, cov_xx, cov_yy;
	WordPair* wp_list;
	WordPair *wp;

  if (argc < 3) {
    printf("Usage: ./distance <VECFILE> <WORDPAIR-LIST> \nwhere VECFILE contains word projections in the BINARY/VEC FORMAT and WORDPAIR-LIST contains a list of word pairs\n");
    return 0;
  }

	wp_list = (WordPair*) malloc (sizeof(WordPair) * MAX_LINES);
	if (!wp_list) {
		fprintf(stderr, "Unable to allocate memory\n");
		return 1;
	}

	vecmode = isVecFile(argv[1]);

  f = fopen(argv[1], "rb");
  if (f == NULL) {
    printf("Input vec file not found\n");
    return -1;
  }

  fscanf(f, "%lld", &words);
  fscanf(f, "%d", &size);

  vocab = (char *)malloc((long long)words * max_w * sizeof(char));
  M = (float *)malloc((long long)words * size * sizeof(float));
  if (M == NULL) {
    printf("Cannot allocate memory: %lld MB    %lld  %d\n", (long long)words * size * sizeof(float) / 1048576, words, size);
    return -1;
  }

  for (b = 0; b < words; b++) {
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
    for (a = 0; a < size; a++) M[a + b * size] /= len;  // normalized
  }
  fclose(f);

  f = fopen(argv[2], "rb");
  if (f == NULL) {
    fprintf(stderr, "Input word-eval file not found\n");
    return -1;
  }

	// Read word pairs and ref sim from data file
	numLines = 0;
	while (fgets(st1, max_size, f) != NULL && numLines < MAX_LINES) {

		token = strtok(st1, sep);
    for (b = words-1; b > 0; b--) {
			if (!strcmp(&vocab[b * max_w], token))
				break;
		}
		if (b==0) continue; // oov
		strcpy(wp_list[numLines].word1, token); 

		token = strtok(NULL, sep);
    for (b = words-1; b > 0; b--) {
			if (!strcmp(&vocab[b * max_w], token))
			break;
		}
		if (b==0) continue; // oov
		strcpy(wp_list[numLines].word2, token); 

		token = strtok(NULL, sep);
		wp_list[numLines].refsim = atof(token);

		numLines++;	
	}
  fclose(f);

	// Sort the file by ref sim values and compute ranks based on this sim
  qsort(wp_list, numLines, sizeof(WordPair), wordpair_compare_refsim); // sort by ref-sim

	rank = 0;
	prev_sim = 0;
	for (i=0; i < numLines; i++) {
		wp = &wp_list[i];
		if (wp->refsim != prev_sim) {
			rank++;
		}
		prev_sim = wp->refsim;
		wp->ref_rank = rank; 
	}

	// compute predicted similarities from embedding space
	for (i=0; i < numLines; i++) {
		wp = &wp_list[i];

    for (b = 0; b < words; b++) if (!strcmp(&vocab[b * max_w], wp->word1)) break;
    for (a = 0; a < words; a++) if (!strcmp(&vocab[a * max_w], wp->word2)) break;
		if (!(a < words && b < words)) {
			fprintf(stderr, "Word pairs (%s, %s) not found\n", wp->word1, wp->word2);
			continue;
		}

		dist = 0;
    for (c = 0; c < size; c++) {
			dist += M[a*size + c] * M[b*size + c];
		}

		wp->sim = dist;
	}

	// Sort by predicted sims and compute ranks
  qsort(wp_list, numLines, sizeof(WordPair), wordpair_compare_predsim); // sort by pred-sim
	rank = 0;
	prev_sim = 0;
	for (i=0; i < numLines; i++) {
		wp = &wp_list[i];
		if (wp->sim != prev_sim) {
			rank++;
		}
		prev_sim = wp->sim;
		wp->pred_rank = rank; 
	}

	sum_rank_ref = 0; sum_rank_pred = 0;

	for (i=0; i < numLines; i++) {
		wp = &wp_list[i];
		printf("%s %s %f %f %d %d\n", wp->word1, wp->word2, wp->sim, wp->refsim, wp->pred_rank, wp->ref_rank);

		sum_rank_ref += wp->ref_rank;
		sum_rank_pred += wp->pred_rank;
	}
	mean_rank_ref = sum_rank_ref/(float)numLines;
	mean_rank_pred = sum_rank_pred/(float)numLines;

	cov_xy = 0;
	cov_xx = 0;
	cov_yy = 0;
	for (i=0; i < numLines; i++) {
		wp = &wp_list[i];

		del_x = wp->ref_rank - mean_rank_ref;
		del_y = wp->pred_rank - mean_rank_pred;

		cov_xy += del_x * del_y;
		cov_xx += del_x * del_x;
		cov_yy += del_y * del_y;
	}

	rho = cov_xy/(sqrt(cov_xx) * sqrt(cov_yy));
	printf("Spearman's rho = %.4f\n", rho);  

  return 0;
}
