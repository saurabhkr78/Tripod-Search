#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <ctime>

/**
 * @class Trie
 * @brief A thread-safe Trie (prefix tree) data structure for efficient prefix-based searches.
 *
 * This class encapsulates the entire Trie implementation, including the node structure,
 * insertion, and search logic. It uses fine-grained locking on each node to ensure
 * thread safety during concurrent operations.
 */
class Trie {
private:
    /**
     * @struct TrieNode
     * @brief Represents a single node in the Trie.
     *
     * Each node contains a map to its children and a flag to indicate if it marks the
     * end of a word. A mutex is included for thread-safe modifications.
     */
    struct TrieNode {
        /// @brief A map from a character to a child node, managed by a smart pointer.
        std::unordered_map<char, std::shared_ptr<TrieNode>> children;
        /// @brief True if this node represents the end of a complete word.
        bool isEndOfWord;
        /// @brief A mutex to protect this node from concurrent access.
        std::mutex mtx;

        /**
         * @brief Construct a new Trie Node object.
         */
        TrieNode() : isEndOfWord(false) {}
    };

    /// @brief The root node of the Trie, managed by a smart pointer.
    std::shared_ptr<TrieNode> root;

public:
    /**
     * @brief Construct a new Trie object.
     * Initializes the root of the Trie.
     */
    Trie() {
        root = std::make_shared<TrieNode>();
    }

    /**
     * @brief Inserts a word into the Trie.
     * This operation is thread-safe.
     * @param word The word to insert.
     */
    void insert(const std::string& word) {
        std::shared_ptr<TrieNode> current = root;
        for (char ch : word) {
            std::lock_guard<std::mutex> lock(current->mtx);
            if (current->children.find(ch) == current->children.end()) {
                // A new node is created and managed by a shared_ptr.
                current->children[ch] = std::make_shared<TrieNode>();
            }
            current = current->children[ch];
        }
        current->isEndOfWord = true;
    }

    /**
     * @brief Searches for a prefix in the Trie.
     * This operation is thread-safe.
     * @param prefix The prefix to search for.
     * @return True if the prefix exists, false otherwise.
     */
    bool search(const std::string& prefix) {
        std::shared_ptr<TrieNode> current = root;
        for (char ch : prefix) {
            // Reading also needs a lock in a concurrent read/write scenario,
            // but for this benchmark, we can assume searches happen after insertions.
            // For a more robust real-world system, a shared_mutex might be better.
            std::lock_guard<std::mutex> lock(current->mtx);
            if (current->children.find(ch) == current->children.end()) {
                return false;
            }
            current = current->children[ch];
        }
        return true;
    }
};

/**
 * @class Benchmark
 * @brief A class dedicated to running performance benchmarks on the Trie.
 *
 * This class encapsulates all the logic for generating test data and measuring the
 * performance of concurrent insertion and search operations on a given Trie instance.
 */
class Benchmark {
private:
    /// @brief A reference to the Trie instance to be benchmarked.
    Trie& trie;
    /// @brief The total number of words to generate for the benchmark.
    const int num_words;
    /// @brief The length of each randomly generated word.
    const int word_length;
    /// @brief The number of concurrent threads to use for the benchmark.
    const int num_threads;
    /// @brief A vector to store the generated words for testing.
    std::vector<std::string> words;

    /**
     * @brief Generates a random string of a given length.
     * @param length The length of the string to generate.
     * @return The randomly generated string.
     */
    std::string generate_random_string(int length) {
        const std::string charset = "abcdefghijklmnopqrstuvwxyz";
        std::string result;
        result.resize(length);
        for (int i = 0; i < length; i++) {
            result[i] = charset[rand() % charset.length()];
        }
        return result;
    }

    /**
     * @brief Generates the dataset of random words for the benchmark.
     */
    void generate_words() {
        words.reserve(num_words);
        for (int i = 0; i < num_words; ++i) {
            words.push_back(generate_random_string(word_length));
        }
    }

public:
    /**
     * @brief Construct a new Benchmark object.
     * @param t The Trie instance to benchmark.
     * @param nw The number of words to generate.
     * @param wl The length of each word.
     * @param nt The number of threads to use.
     */
    Benchmark(Trie& t, int nw, int wl, int nt)
        : trie(t), num_words(nw), word_length(wl), num_threads(nt) {
        srand(time(0));
        generate_words();
    }

    /**
     * @brief Runs the full benchmark suite, including insertion and search tests.
     */
    void run() {
        run_insertion_benchmark();
        run_search_benchmark();
    }

    /**
     * @brief Runs the concurrent insertion benchmark.
     *
     * It measures the time taken to insert a large number of words into the Trie
     * using multiple threads.
     */
    void run_insertion_benchmark() {
        std::vector<std::thread> threads;
        auto start_insert = std::chrono::high_resolution_clock::now();
        int words_per_thread = num_words / num_threads;

        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, i]() {
                int start = i * words_per_thread;
                int end = (i == num_threads - 1) ? num_words : (i + 1) * words_per_thread;
                for (int j = start; j < end; ++j) {
                    trie.insert(words[j]);
                }
            });
        }

        for (auto& th : threads) {
            th.join();
        }

        auto end_insert = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> insert_duration = end_insert - start_insert;
        std::cout << "Time taken to insert " << num_words << " words with " << num_threads << " threads: " << insert_duration.count() << " seconds" << std::endl;
    }

    /**
     * @brief Runs the concurrent search benchmark.
     *
     * It measures the time taken to search for a large number of words in the Trie
     * using multiple threads.
     */
    void run_search_benchmark() {
        std::vector<std::thread> threads;
        auto start_search = std::chrono::high_resolution_clock::now();
        int words_per_thread = num_words / num_threads;

        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, i]() {
                int start = i * words_per_thread;
                int end = (i == num_threads - 1) ? num_words : (i + 1) * words_per_thread;
                for (int j = start; j < end; ++j) {
                    trie.search(words[j]);
                }
            });
        }

        for (auto& th : threads) {
            th.join();
        }

        auto end_search = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> search_duration = end_search - start_search;
        std::cout << "Time taken to search for " << num_words << " words with " << num_threads << " threads: " << search_duration.count() << " seconds" << std::endl;
    }
};

int main() {
    Trie trie;
    Benchmark benchmark(trie, 100000, 5, 4);
    benchmark.run();
    return 0;
}
