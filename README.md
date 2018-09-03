# skip-gram with negative sampling with subspace projection (SP-SGNS)

Learn to embed words without any pre-set parameter value for the dimensionality of each vector. The only parameter
is the dimension of the overall space, of which each embedded vector is a projection. 
The dimension of each vector is a function of the norm of the classification error vector. Consequently, words
which can be well classified given a context tend to have low number of dimensions in an embedded space of
maximium number of dimensions (rest being 0s). Whereas words that are more ambiguous (e.g. polysemous or function words)
tend to be embedded with higher number of dimensions.

This also ensures that the embedding space can be stored in a more compact form with less storage space as compared
to embedding spaces with fixed number of dimensions.

## Implementation

An easy-to-use sample script is provided which is invoked as follows
``
./semeval.sh i>min-size> <max-size (>min-size)> <context-size> <negative sampling size>>
``
e.g.
``
./semeval.sh 100 300 10 5 
``
which embeds vectors in a space of max 300 dimensions (minimum dimension being 100), uses word2vec window size of 10 and 5 negative samples for skip-gram. 
