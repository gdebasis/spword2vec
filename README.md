# SP-SGNS

## Implementation

An easy-to-use sample script is provided which is invoked as follows

``
./semeval.sh <min-size> <max-size (> min-size)> <context-size> <negative sampling size>>
``

A sample invocation is

``
./semeval.sh 100 300 10 5 
``

which embeds vectors in a space of max 300 dimensions (minimum dimension being 100), uses word2vec window size of 10 and 5 negative samples for skip-gram. 
