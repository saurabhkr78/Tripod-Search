Tripod Search - Trie Data Structure Implementation
Overview

Tripod Search is an in-memory prefix search system built using a Trie data structure. It is designed to efficiently handle prefix-based searches and insertions. The project includes performance benchmarking for both insertion and search operations, with thread-safety mechanisms implemented using mutex for concurrent access.
Features

    Prefix Search: Efficiently search for words starting with a given prefix using the Trie data structure.

    Thread-Safety: Ensures safe concurrent access by using fine-grained locking via mutex.

    Performance Benchmarking: Measures the time taken for large-scale insertions and searches (100,000 words in this case).
