// Copyright 2013 Google Inc. All Rights Reserved.
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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_STRING 100
#define EXP_TABLE_SIZE 1000
#define MAX_EXP 6
#define MAX_LINE_SIZE 10000
#define MAX_SENTENCE_LENGTH 1000
#define MAX_CODE_LENGTH 40
#define MAX_OUT_DEGREE 5000
#define MAX_CONTEXT_PATH_LEN 40

const int vocab_hash_size = 300000;  // Maximum 30 * 0.7 = 21M words in the vocabulary

typedef float real;                    // Precision of float numbers

struct vocab_node;
struct edge;

typedef char byte;

typedef struct edge {
	struct vocab_node* dest;
	real weight;
	byte twohop;  // 1 if two-hop	
}
edge;

edge ***multiHopEdgeLists;

// represents a node structure
typedef struct vocab_node {
	int id;    // the id (hash index) of the word
  char *word;
	edge *edge_list;
  int cn;  // out degree
	byte visited;
} vocab_node;

char train_file[MAX_STRING], output_file[MAX_STRING], output_file_vec[MAX_STRING], output_file_bin[MAX_STRING];
char pretrained_file[MAX_STRING];
char lineBuff[MAX_LINE_SIZE];	
vocab_node *vocab;
int debug_mode = 2, window = 10, min_count = 0;
int *vocab_hash;
int vocab_max_size = 1000, vocab_size = 0, layer1_size = 100;
int train_nodes = 0, iter = 5, gfmt = 2, directed = 1;
real alpha = 0.025, sample = 1e-3;
real *syn0, *syn1, *syn1neg, *expTable;
int max_path_length = 2; // maximum path length of random walk... set this to either 1 or 2.
real max_weight = 0;
int negative = 5;
const int table_size = 1e8;
int *table;

void InitUnigramTable() {
  int a, i;
  int train_nodes_pow = 0;
  real d1, power = 0.75;
  table = (int *)malloc(table_size * sizeof(int));
  for (a = 0; a < vocab_size; a++) train_nodes_pow += pow(vocab[a].cn, power);
  i = 0;
  d1 = pow(vocab[i].cn, power) / (real)train_nodes_pow;
  for (a = 0; a < table_size; a++) {
    table[a] = i;
    if (a / (real)table_size > d1) {
      i++;
      d1 += pow(vocab[i].cn, power) / (real)train_nodes_pow;
    }
    if (i >= vocab_size) i = vocab_size - 1;
  }
}

// Reads a single word from a file, assuming space + tab + EOL to be word boundaries
void ReadWord(char *word, FILE *fin) {
  int a = 0, ch;
  while (!feof(fin)) {
    ch = fgetc(fin);
    if (ch == 13) continue;
    if ((ch == ' ') || (ch == '\t') || (ch == '\n')) {
      if (a > 0) {
        if (ch == '\n') ungetc(ch, fin);
        break;
      }
      if (ch == '\n') {
        strcpy(word, (char *)"</s>");
        return;
      } else continue;
    }
    word[a] = ch;
    a++;
    if (a >= MAX_STRING - 1) a--;   // Truncate too long words
  }
  word[a] = 0;
}

// Reads the node id from a file... stops reading when it sees a tab character.
// Each line in the graph file is: <src node>\t[<dest-node>:<weight>]*
void ReadSrcNode(char *word, FILE *fin) {
  int a = 0, ch;
  while (!feof(fin)) {
    ch = fgetc(fin);
    if (ch == '\t')
    {
	    break;
    }
    if ((ch == ' ') || (ch == '\t') || (ch == '\n')) {
      if (a > 0) {
        if (ch == '\n') ungetc(ch, fin);
        break;
      }
      if (ch == '\n') {
        strcpy(word, (char *)"</s>");
        return;
      } else continue;
    }
    word[a] = ch;
    a++;
    if (a >= MAX_STRING - 1) a--;   // Truncate too long words
  }
  word[a] = 0;
	// skip characters to put the file pointer to the start of next line
	while (!feof(fin)) {
  	while ((ch = fgetc(fin)) != '\n');
    	break;
  }
}

// Returns hash value of a word
int GetWordHash(char *word) {
  unsigned int a, hash = 0;
  for (a = 0; a < strlen(word); a++) hash = hash * 257 + word[a];
  hash = hash % vocab_hash_size;
  return hash;
}

// Returns position of a word in the vocabulary; if the word is not found, returns -1
int SearchVocab(char *word) {
  unsigned int hash = GetWordHash(word);
  while (1) {
    if (vocab_hash[hash] == -1) return -1;
    if (!strcmp(word, vocab[vocab_hash[hash]].word)) return vocab_hash[hash];
    hash = (hash + 1) % vocab_hash_size;
  }
  return -1;
}

// Adds a word to the vocabulary
int AddWordToVocab(char *word) {
  unsigned int hash, id, length = strlen(word) + 1;
  if (length > MAX_STRING) length = MAX_STRING;
  vocab[vocab_size].word = (char *)calloc(length, sizeof(char));
  strcpy(vocab[vocab_size].word, word);
  vocab[vocab_size].cn = 0;
  vocab[vocab_size].visited = 0;

  vocab_size++;
  // Reallocate memory if needed
  if (vocab_size + 2 >= vocab_max_size) {
    vocab_max_size += 1000;
    vocab = (struct vocab_node *)realloc(vocab, vocab_max_size * sizeof(struct vocab_node));
  }
  hash = GetWordHash(word);
  while (vocab_hash[hash] != -1) hash = (hash + 1) % vocab_hash_size;

	id = vocab_size-1;
  vocab_hash[hash] = id;
	// vocab_size-1 is the index of the current word... save it in the node object
	vocab[id].id = id;
  //printf("\n%s Adding word",word); 
  return id;
}

// Used later for sorting by out degrees
int VocabCompare(const void *a, const void *b) {
    return ((struct vocab_node *)b)->cn - ((struct vocab_node *)a)->cn;
}

// Sorts the vocabulary by frequency using word counts
void SortVocab() {
  int a, size;
  unsigned int hash;
  // Sort the vocabulary and keep </s> at the first position
  qsort(&vocab[1], vocab_size - 1, sizeof(struct vocab_node), VocabCompare);
  for (a = 0; a < vocab_hash_size; a++) vocab_hash[a] = -1;
  size = vocab_size;
  train_nodes = 0;
  for (a = 0; a < size; a++) {
    // Nodes with out-degree less than min_count times will be discarded from the vocab
    if ((vocab[a].cn < min_count) && (a != 0)) {
      vocab_size--;
      free(vocab[a].word);
    } else {
      // Hash will be re-computed, as after the sorting it is not actual
      hash=GetWordHash(vocab[a].word);
      while (vocab_hash[hash] != -1) hash = (hash + 1) % vocab_hash_size;
      vocab_hash[hash] = a;
      train_nodes += vocab[a].cn;
    }
  }
  vocab = (struct vocab_node *)realloc(vocab, (vocab_size + 1) * sizeof(struct vocab_node));
}

// Stores a list of pointers to edges for each source node.
// The overall list is thus a pointer to pointer to the lists.
int initContexts() {
	int i;

	// a list of contexts for each source node in the graph
	multiHopEdgeLists = (edge***) malloc (sizeof(edge**) * vocab_size);
	if (multiHopEdgeLists == NULL) {
		fprintf(stderr, "Unable to allocate memory to save the list of contexts.\n");
	
		return 0;
	}

	for (i=0; i < vocab_size; i++) {
		multiHopEdgeLists[i] = (edge**) malloc (sizeof(edge*) * MAX_CONTEXT_PATH_LEN);
		if (multiHopEdgeLists[i] == NULL) {
			fprintf(stderr, "Unable to allocate memory to save the contexts.\n");
			return 0;
		}
	}
	return 1;
}

int constructGraph_adjList(FILE* fp);
int constructGraph_edgeList(FILE* fp);

int constructGraph(FILE* fp) {
	if (gfmt == 1)
		return constructGraph_adjList(fp);
	return constructGraph_edgeList(fp);
}

int addEdge(char* src, char* dest, float wt) {
		int src_node_index, dst_node_index, cn;
		edge* edge_list;

		// Get src node id
		src_node_index = SearchVocab(src);

		// Get dst node id
		dst_node_index = SearchVocab(dest);
		
		// allocate edges

		edge_list = vocab[src_node_index].edge_list;
  	if (edge_list == NULL) {
			edge_list = (edge*) malloc (sizeof(edge) * MAX_OUT_DEGREE);
			cn = 0;
		}
		else {
			cn = vocab[src_node_index].cn; // current number of edges
		}

		if (edge_list == NULL)
			return 0;

		if (cn == MAX_OUT_DEGREE) {
			fprintf(stderr, "Can't add anymore edges...\n");
			return 0;
		}

		edge_list[cn].dest = &vocab[dst_node_index]; 
	 	edge_list[cn].dest->id = dst_node_index; 
		edge_list[cn].weight = wt;

		vocab[src_node_index].cn = cn+1; // number of edges
		return 1;
}

// Each line represents an edge...
// format is <src-node-id> \t <dest-node-id> \t <weight>
// supports the option of directed/undirected...
// for undirected option, the function adds the reverse edges
int constructGraph_edgeList(FILE* fp) {
	int i;
	char *src_word, *dst_word, *wt_word;
	float wt;

	while (fgets(lineBuff, sizeof(lineBuff), fp)) {
		i = 0;

		src_word = lineBuff;
		while (*(lineBuff + i) != '\t') { 
			i++;
		}
  	*(lineBuff + i) = '\0';

		i++; // skip the tab character

		dst_word = lineBuff+i;
		while (*(lineBuff + i) != '\t') { 
			i++;
		}

  	*(lineBuff + i) = '\0';

		i++; // skip the tab character
		wt_word = lineBuff+i;
		while (*(lineBuff + i) != '\n') { 
			i++;
		}
  	*(lineBuff + i) = '\0';

		wt = atof(wt_word);	

		if (!addEdge(src_word, dst_word, wt)) return 0;  // add this edge to G
		if (directed)
			if (!addEdge(dst_word, src_word, wt)) return 0;  // add the reverse edge to G
	}
	return 1;
}

// for the adjacency list option, the program doesn't support
// the directed/undirected option
int constructGraph_adjList(FILE* fp) {
	int i = 0, pos = 0, j = 0, src_node_index, node_index, count = 0;
	edge* edge_list;
	char *word;

	while (fgets(lineBuff, sizeof(lineBuff), fp)) {
		// allocate edges
		edge_list = (edge*) malloc (sizeof(edge) * MAX_OUT_DEGREE);
		if (edge_list == NULL)
			return 0;
		while (*(lineBuff + i) != '\t') { 
			i++;
		}

  	*(lineBuff + i) = '\0';
		i++; // skip the tab character
                 
		src_node_index = SearchVocab(lineBuff);
 		if (src_node_index < 0) {
  		i = 0;
			continue;
  	}
		word = strtok(lineBuff+i, " ");
  	j = 0;
  	while (word != NULL) {
  		pos = 0;
			count = count +1;	
			while (*(word + pos) != ':') pos++; 
			*(word + pos) = 0;

	    node_index = SearchVocab(word);
  	  if (node_index > 0) {
				edge_list[j].dest=&vocab[node_index]; 
	    	edge_list[j].dest->id = node_index; 
				edge_list[j].weight = atof(word+pos+1);	
				j++; 
			}
    	word = strtok(NULL, " ");
    	if (strcmp(word, "\n") == 0) {
				i = 0;
      	break;
			}
		}
  	vocab[src_node_index].edge_list = edge_list;
		vocab[src_node_index].cn = j; // number of edges
  	i = 0;
	}

	return 1;  
}

// an important step is to normalize the edge weights to probabilties
// of samples that would be used later on during sampling nodes
// from this pre-built context.
void preComputePathContextForSrcNode(int src_node_index) {
	int i = 0, j;  // index into the context buffer
	edge *p, *q;
	edge **multiHopEdgeList;
	vocab_node *src_node;
	real z = 0;
	static const real onehop_pref = 0.7;
	static const real one_minus_onehop_pref = 1 - onehop_pref;

	src_node = &vocab[src_node_index];
	multiHopEdgeList = multiHopEdgeLists[src_node_index]; // write to the correct buffer
	p = src_node->edge_list;

 	// collect a set of 2 hop nodes from this source node 
	for (p = src_node->edge_list; p < &src_node->edge_list[src_node->cn]; p++) {

		// visit a one-hop node from source
		if (!p->dest->visited && i < MAX_CONTEXT_PATH_LEN) {
   		multiHopEdgeList[i++] = p;
			p->twohop = 0;
			p->dest->visited = 1;
		}

		// for each visited one-hop node, visit its one-hop neighbors
		for (q = p->dest->edge_list; q < &p->dest->edge_list[p->dest->cn]; q++) {
    	// visit a one-hop node from source
      if (!q->dest->visited && i < MAX_CONTEXT_PATH_LEN) {
				multiHopEdgeList[i++] = q;
				q->twohop = 1;
				q->dest->visited = 1;
	   	}
		}
	}
	multiHopEdgeList[i] = NULL;

	z = 0;
	// reset the visited flags (for next call to the function)
	for (j = 0; j < i; j++) {
		multiHopEdgeList[j]->weight *= multiHopEdgeList[j]->twohop? one_minus_onehop_pref : onehop_pref;  // prob of one-hop vs two-hop
		multiHopEdgeList[j]->dest->visited = 0;
		z += multiHopEdgeList[j]->weight;
	}

	for (j = 0; j < i; j++) {
		multiHopEdgeList[j]->weight /= z;
	}
}

// Precompute the set of max-hop nodes for each source node.
int preComputePathContexts() {
	int i;

	if (!initContexts())
		return 0;

	for (i=0; i < vocab_size; i++) {
		preComputePathContextForSrcNode(i);
	}	
	return 1;
}

// Sample a context of size <window>
// contextBuff is an o/p parameter
int sampleContext(int src_node_index, unsigned long next_random, edge** contextBuff) {
	edge **multiHopEdgeList;
	edge **p, *tmp;
	int len = MAX_CONTEXT_PATH_LEN, i, j = 0, orig_len;
	real x, cumul_p;
  vocab_node src_node;	
	src_node = vocab[src_node_index];
	// see how many 2-hop adj neighbors we have got for this node 

	multiHopEdgeList = multiHopEdgeLists[src_node_index]; // buffer to sample from
	for (p = multiHopEdgeList; p < &multiHopEdgeList[MAX_CONTEXT_PATH_LEN]; p++) {
		if (*p == NULL)
			break;
	}
	len = p - multiHopEdgeList;
	
	len = window < len? window: len;
	orig_len = len;

	if (debug_mode > 2)
		printf("Original Word %s: ", src_node.word);

	memset(contextBuff, 0, sizeof(edge*)*len);

	// normalize the weights so that they sum to 1.
	cumul_p = 0;
	for (p = multiHopEdgeList; p < &multiHopEdgeList[len]; p++) {
		cumul_p += (*p)->weight;
	}

	// divide by Z (norm constant)
	for (p = multiHopEdgeList; p < &multiHopEdgeList[len]; p++) {
		(*p)->weight = (*p)->weight / cumul_p;
	}
	
	for (i=0; i < len; i++) {  // ith sample
		cumul_p = 0;
  	next_random = next_random * (unsigned long)25214903917 + 11;
		x = ((next_random & 0xFFFF) / (real)65536);  // [0, 1]

		// Find out in which interval does this belong to...
		for (p = multiHopEdgeList; p < &multiHopEdgeList[len]; p++) {
			if (cumul_p <= x && x < cumul_p + (*p)->weight)
				break;	

			cumul_p += (*p)->weight;
		}

		// save sampled nodes in context
		contextBuff[j++] = *p;

		// swap the selected element with the last and decrease array size --> no duplicate samples
		tmp = multiHopEdgeList[len];
		multiHopEdgeList[len] = *p;
		*p = tmp;
		len--;
	}

	return orig_len;
}

// Each line in this graph file is of the following format:
// <src-node-id>\t [<dest-node-id>:<weight of this edge> ]* 
int LearnVocabFromTrainFile() {
  char src_node_id[MAX_STRING];
  FILE *fin;
  int a, i;

	printf("Loading nodes from graph file...\n");	
  printf("%d\n", vocab_hash_size);
  for (a = 0; a < vocab_hash_size; a++) vocab_hash[a] = -1;
  fin = fopen(train_file, "rb");
  if (fin == NULL) {
    printf("ERROR: training data file not found!\n");
    exit(1);
  }
  vocab_size = 0;
  
	while (1) {
    ReadSrcNode(src_node_id, fin);
    if (feof(fin)) break;
    train_nodes++;
    i = SearchVocab(src_node_id);
    if (i == -1) {
      a = AddWordToVocab(src_node_id);
      vocab[a].cn = 1;
    } else vocab[i].cn++;
  }
  SortVocab();
  if (debug_mode > 0) {
    printf("#nodes: %d\n", vocab_size);
  }
  fin = fopen(train_file, "rb");
   if (!constructGraph(fin))
		return 0; 
	fclose(fin);

	if (!preComputePathContexts())
		return 0;
	return 1;
}

int InitNet() { 
  int a, b, wordIndex;
	int binFileNumWords, binFileVecSize;
  unsigned long next_random = 1;
  char word[MAX_STRING];

  FILE *f;
  const int max_size = 2000;         // max length of strings
  char file_name[max_size];

  a = posix_memalign((void **)&syn0, 128, (int)vocab_size * layer1_size * sizeof(real));
  if (syn0 == NULL) {printf("Memory allocation failed\n"); exit(1);}

  if (negative>0) {
    a = posix_memalign((void **)&syn1neg, 128, (int)vocab_size * layer1_size * sizeof(real));
    if (syn1neg == NULL) {printf("Memory allocation failed\n"); exit(1);}
    for (a = 0; a < vocab_size; a++) for (b = 0; b < layer1_size; b++)
     syn1neg[a * layer1_size + b] = 0;
  }

	// Initialize the net weights from a pretrained model
	// The file to be loaded is a binary file saved by word2vec. 
	// This is to ensure that the word vectors will not be
	// trained... only the doc vectors will be
  if (*pretrained_file) { 
		strcpy(file_name, pretrained_file);
  	f = fopen(file_name, "rb");
		if (f == NULL) {
	    printf("Input file not found\n");
	    return -1;
	  }

		// read the vocabulary size and dimensions
  	fscanf(f, "%d", &binFileNumWords);
  	fscanf(f, "%d", &binFileVecSize);

		if (layer1_size != binFileVecSize) {
			printf("Mismatch in required number of dimensions with that of the saved bin file!\n"); 
			return -1;
		}
	
  	for (b = 0; b < binFileNumWords; b++) {
  		// first entry is a word
    	ReadWord(word, f);

			// We have loaded vocab from the current training file.
			// If we find this word in the current vocab set its
			// vector to the value from file, else do random init
    	wordIndex = SearchVocab(word);
			if (wordIndex >= 0) {
				for (b = 0; b < layer1_size; b++)
    			fread(&syn0[wordIndex * layer1_size + b], sizeof(real), 1, f);
			}
		}
		fclose(f);   // close the bin file
  }
  else { // Random initialization in absence of pre-trained file
    for (a = 0; a < vocab_size; a++) for (b = 0; b < layer1_size; b++) {
      next_random = next_random * (unsigned long)25214903917 + 11;
      syn0[a * layer1_size + b] = (((next_random & 0xFFFF) / (real)65536) - 0.5) / layer1_size;//Initialize by random nos
    }
  }  

	return 0;
}

void skipgram() {
	edge *contextBuff[MAX_CONTEXT_PATH_LEN], *p;  // context sampled for each node
  int word, last_word;
	int a, c, d;
  int l1, l2, target, label; 
	int context_len;
  unsigned long next_random = 123456;
  real f, g;
  real *neu1e = (real *)calloc(layer1_size, sizeof(real));

	for (word=0; word < vocab_size; word++) {
		context_len = sampleContext(word, next_random, contextBuff);  

		// train skip-gram on node contexts
 		for (a = 0; a < context_len; a++) { 
			p = contextBuff[a];
   		last_word = p->dest->id;
			l1 = last_word * layer1_size;
			memset(neu1e, 0, layer1_size * sizeof(real));
        	
			// NEGATIVE SAMPLING
   		if (negative > 0) for (d = 0; d < negative + 1; d++) {
	 			if (d == 0) {
 					target = word;
   				label = 1;
   			} else {
 					next_random = next_random * (unsigned long)25214903917 + 11;
					target = table[(next_random >> 16) % table_size];
       		if (target == 0) target = next_random % (vocab_size - 1) + 1;
       		if (target == word) continue;
         	label = 0;
       	}
       	l2 = target * layer1_size;
       	f = 0;
       	for (c = 0; c < layer1_size; c++) f += syn0[c + l1] * syn1neg[c + l2];
				// compute gradient
       	if (f > MAX_EXP) g = (label - 1) * alpha;
       	else if (f < -MAX_EXP) g = (label - 0) * alpha;
       	else g = (label - expTable[(int)((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2))]) * alpha;

     		for (c = 0; c < layer1_size; c++) neu1e[c] += g * syn1neg[c + l2];
     		for (c = 0; c < layer1_size; c++) syn1neg[c + l2] += g * syn0[c + l1];
			}
	
			// Learn weights input -> hidden
   		for (c = 0; c < layer1_size; c++) syn0[c + l1] += neu1e[c]; 
		} 
  }

	free(neu1e);
}

void train() {
  long a;
	int b;
  FILE *fo, *fo2;
  printf("Starting training using file %s\n", train_file);
  if (!LearnVocabFromTrainFile())
		fprintf(stderr, "Error in loading the graph file\n");

  if (output_file[0] == 0) return;
  InitNet();
  if (negative > 0) InitUnigramTable();
	printf("Unigram table initialized...\n");

	skipgram();

  snprintf(output_file_vec, sizeof(output_file_vec), "%s.vec", output_file); 
  snprintf(output_file_bin, sizeof(output_file_bin), "%s.bin", output_file); 

  fo = fopen(output_file_bin, "wb");
  fo2 = fopen(output_file_vec, "wb");

  // Save the word vectors
  fprintf(fo, "%d %d\n", vocab_size, layer1_size);
  fprintf(fo2, "%d %d\n", vocab_size, layer1_size);
  for (a = 0; a < vocab_size; a++) {
    fprintf(fo, "%s ", vocab[a].word);
    fprintf(fo2, "%s ", vocab[a].word);
		for (b = 0; b < layer1_size; b++) fwrite(&syn0[a * layer1_size + b], sizeof(real), 1, fo);
		for (b = 0; b < layer1_size; b++) fprintf(fo2, "%lf ", syn0[a * layer1_size + b]);
    fprintf(fo, "\n");
    fprintf(fo2, "\n");
  }

	fclose(fo);
  fclose(fo2);
}

int ArgPos(char *str, int argc, char **argv) {
  int a;
  for (a = 1; a < argc; a++) if (!strcmp(str, argv[a])) {
    if (a == argc - 1) {
      printf("Argument missing for %s\n", str);
      exit(1);
    }
    return a;
  }
  return -1;
}

int main(int argc, char **argv) {
  int i;
  if (argc == 1) {
    printf("Node2Vec toolkit v 0.1c\n\n");
    printf("Options:\n");
    printf("Parameters for training:\n");
    printf("\t-train <file>\n");
    printf("Graph format -- {adj=1,edge=2} (default:2) :\n");
    printf("\t-gfmt <format code>\n");
    printf("\t\tGraph file (each line a node: <node-id> \t [<node-id>:<weight>]*)\n");
    printf("\t-pt <file>\n");
    printf("\t\tPre-trained vectors for nodes (word2vec bin file format)\n");
    printf("\t-output <file>\n");
    printf("\t\tUse <file> to save the resulting word vectors / word clusters\n");
    printf("\t-size <int>\n");
    printf("\t\tSet size of word vectors; default is 100\n");
    printf("\t-window <int>\n");
    printf("\t\tContext (random walk) length.\n");
    printf("\t-negative <int>\n");
    printf("\t\tNumber of negative examples; default is 5, common values are 3 - 10 (0 = not used)\n");
    printf("\t-iter <int>\n");
    printf("\t\tRun more training iterations (default 5)\n");
    printf("\t-min-count <int>\n");
    printf("\t\tNodes with out-degree less than min-count are discarded; default is 5\n");
    printf("\t-alpha <float>\n");
    printf("\t\tSet the starting learning rate; default is 0.025 for skip-gram\n");
    printf("\t-directed <0/1>\n");
    printf("\t\twhether the graph is directed (if undirected, reverse edges are automatically added when the i/p fmt is edge list>\n");
    printf("\nExample:\n");
    printf("./node2vec -pt nodes.bin -train graph.txt -gfmt 2 -output ovec -size 200 -window 5 -sample 1e-4 -negative 5 -iter 3\n\n");

    return 0;
  }
  output_file[0] = 0;
	pretrained_file[0] = 0;
  if ((i = ArgPos((char *)"-size", argc, argv)) > 0) layer1_size = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-train", argc, argv)) > 0) strcpy(train_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-alpha", argc, argv)) > 0) alpha = atof(argv[i + 1]);
  if ((i = ArgPos((char *)"-output", argc, argv)) > 0) strcpy(output_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-gfmt", argc, argv)) > 0) gfmt = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-directed", argc, argv)) > 0) directed = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-pt", argc, argv)) > 0) strcpy(pretrained_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-window", argc, argv)) > 0) window = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-sample", argc, argv)) > 0) sample = atof(argv[i + 1]);
  if ((i = ArgPos((char *)"-negative", argc, argv)) > 0) negative = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-iter", argc, argv)) > 0) iter = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-min-count", argc, argv)) > 0) min_count = atoi(argv[i + 1]);

	if (window > MAX_CONTEXT_PATH_LEN) {
		printf("Window size %d value too large. Truncating the value to %d\n", window, MAX_CONTEXT_PATH_LEN);
		window = MAX_CONTEXT_PATH_LEN;
	}

  vocab = (struct vocab_node *)calloc(vocab_max_size, sizeof(struct vocab_node));
  vocab_hash = (int *)calloc(vocab_hash_size, sizeof(int));
  expTable = (real *)malloc((EXP_TABLE_SIZE + 1) * sizeof(real));
  for (i = 0; i < EXP_TABLE_SIZE; i++) {
    expTable[i] = exp((i / (real)EXP_TABLE_SIZE * 2 - 1) * MAX_EXP); // Precompute the exp() table
    expTable[i] = expTable[i] / (expTable[i] + 1);                   // Precompute f(x) = x / (x + 1)
  }
  train();
  return 0;
}
