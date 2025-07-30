# Tripod Search - A Thread-Safe Trie-Based Prefix Search Engine

## Elevator Pitch

Tripod Search is a high-performance, in-memory prefix search engine built in C++. It uses a Trie data structure to provide exceptionally fast search and insertion capabilities, making it ideal for applications like autocomplete, spell checkers, and IP routing. The system is fully thread-safe, allowing for safe concurrent operations, and includes built-in benchmarking to demonstrate its efficiency, capable of handling hundreds of thousands of operations per second.

## Problem Solved

In many applications, there is a need to quickly find all items that start with a specific prefix. Traditional search methods, like iterating through a list of strings, are inefficient and scale poorly as the dataset grows. Tripod Search solves this by implementing a Trie, a specialized tree-like data structure that stores strings in a way that makes prefix-based lookups extremely fast, with a time complexity proportional to the length of the prefix, not the size of the dataset.

## My Role & Contributions

As the sole developer on this project, I was responsible for the entire development lifecycle, from design to implementation and testing. My key contributions include:
- **Designing and implementing the core Trie data structure** and its associated node structure.
- **Integrating thread-safety** using mutexes to ensure the data structure can be used in concurrent environments without data corruption.
- **Developing a performance benchmarking suite** to measure and validate the efficiency of insertion and search operations under load.
- **Writing clean, modern C++ code**, utilizing smart pointers for automatic memory management and adhering to object-oriented principles.

## Technology Stack

- **C++11:** The core language used for its performance, control over memory, and powerful Standard Template Library (STL).
- **STL Containers:** `std::unordered_map` was chosen for the children of each Trie node to provide fast, average-case O(1) lookups for the next character. `std::vector` was used to store test data for benchmarking.
- **C++ Multithreading (`<thread>`):** Used to implement concurrent insertion and search operations to test and prove the thread-safety of the Trie.
- **Mutex (`<mutex>`):** The standard library mutex was used to implement fine-grained locking on each Trie node, ensuring data integrity during concurrent write operations.
- **Smart Pointers (`<memory>`):** `std::shared_ptr` was used to manage the lifecycle of `TrieNode` objects, preventing memory leaks and simplifying memory management (RAII).

## Architecture & Design Patterns

- **Trie Data Structure:** The fundamental architecture is based on the Trie (Prefix Tree). Each node in the trie represents a character, and a path from the root to a node represents a prefix. This is a highly specialized and efficient pattern for prefix-based operations.
- **Object-Oriented Programming (OOP):** The system is designed using OOP principles. The `Trie` and `TrieNode` are implemented as separate classes/structs, encapsulating their own data and logic.
- **RAII (Resource Acquisition Is Initialization):** By using `std::shared_ptr` for nodes and `std::lock_guard` for mutexes, the project leverages RAII to ensure that memory is automatically deallocated and mutexes are always released, even in the presence of exceptions. This leads to safer and more robust code.

## Object-Oriented Implementation

The project demonstrates several key OOP concepts:
- **Encapsulation:** The `Trie` class encapsulates the logic for `insert` and `search` operations, hiding the complex internal implementation from the user. The `root` node is private, so it cannot be manipulated directly.
- **Abstraction:** The `Trie` class provides a simple interface (`insert(word)`, `search(prefix)`) that abstracts away the underlying complexity of the Trie data structure.
- **Data Structures as Objects:** Both `Trie` and `TrieNode` are defined as objects (a `class` and a `struct`, respectively), which bundle data (e.g., `children`, `isEndOfWord`) and behavior together.

## Measurable Outcomes & Achievements

The primary achievement of this project is the creation of a highly efficient and thread-safe prefix search system.
- **Performance:** The benchmarking results demonstrate the system's high throughput. On a typical machine, it can **insert 100,000 words in under a second** and **search for 100,000 prefixes in approximately 0.2 seconds** using 4 threads.
- **Thread Safety:** The implementation successfully uses fine-grained mutexes to allow for safe and correct concurrent operations, a critical requirement for modern, multi-threaded applications.
- **Scalability:** The search time complexity is O(L), where L is the length of the prefix, making it highly scalable as the number of words in the dictionary grows.
