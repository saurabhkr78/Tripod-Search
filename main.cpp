#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <shared_mutex>
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
        /// @brief A map from a character to a child node, managed by a unique pointer.
        std::unordered_map<char, std::unique_ptr<TrieNode>> children;
        /// @brief True if this node represents the end of a complete word.
        bool isEndOfWord;
        /// @brief A shared mutex to protect this node from concurrent access (many readers, one writer).
        mutable std::shared_mutex mtx;

        /**
         * @brief Construct a new Trie Node object.
         */
        TrieNode() : isEndOfWord(false) {}
    };

    /// @brief The root node of the Trie, managed by a unique pointer.
    std::unique_ptr<TrieNode> root;

public:
    /**
     * @brief Construct a new Trie object.
     * Initializes the root of the Trie.
     */
    Trie() {
        root = std::make_unique<TrieNode>();
    }

    /**
     * @brief Inserts a word into the Trie.
     * This operation is thread-safe.
     * @param word The word to insert.
     */
    void insert(const std::string& word) {
        TrieNode* current = root.get();
        for (char ch : word) {
            std::unique_lock<std::shared_mutex> lock(current->mtx);
            if (current->children.find(ch) == current->children.end()) {
                current->children[ch] = std::make_unique<TrieNode>();
            }
            current = current->children[ch].get();
        }
        std::unique_lock<std::shared_mutex> lock(current->mtx);
        current->isEndOfWord = true;
    }

    /**
     * @brief Searches for a prefix in the Trie.
     * This operation is thread-safe.
     * @param prefix The prefix to search for.
     * @return True if the prefix exists, false otherwise.
     */
    bool search(const std::string& prefix) {
        TrieNode* current = root.get();
        for (char ch : prefix) {
            std::shared_lock<std::shared_mutex> lock(current->mtx);
            if (current->children.find(ch) == current->children.end()) {
                return false;
            }
            current = current->children[ch].get();
        }
        std::shared_lock<std::shared_mutex> lock(current->mtx);
        return current->isEndOfWord;
    }

    /**
     * @brief Removes a word from the Trie.
     * This operation is thread-safe.
     * @param word The word to remove.
     */
    void remove(const std::string& word) {
        remove_helper(root.get(), word, 0);
    }

    /**
     * @brief Returns all words in the Trie with a given prefix.
     * @param prefix The prefix to search for.
     * @return A vector of strings containing all words with the given prefix.
     */
    std::vector<std::string> words_with_prefix(const std::string& prefix) {
        TrieNode* current = root.get();
        for (char ch : prefix) {
            std::shared_lock<std::shared_mutex> lock(current->mtx);
            if (current->children.find(ch) == current->children.end()) {
                return {};
            }
            current = current->children[ch].get();
        }

        std::vector<std::string> results;
        collect_words(current, prefix, results);
        return results;
    }

private:
    /**
     * @brief Helper function to recursively collect words from a given node.
     * @param node The starting node.
     * @param current_prefix The prefix accumulated so far.
     * @param results The vector to store the found words.
     */
    void collect_words(TrieNode* node, std::string current_prefix, std::vector<std::string>& results) {
        std::shared_lock<std::shared_mutex> lock(node->mtx);
        if (node->isEndOfWord) {
            results.push_back(current_prefix);
        }

        for (const auto& pair : node->children) {
            collect_words(pair.second.get(), current_prefix + pair.first, results);
        }
    }

    /**
     * @brief Helper function to recursively remove a word from the Trie.
     * @param current The current node.
     * @param word The word to remove.
     * @param depth The current depth in the Trie.
     * @return True if the current node can be deleted, false otherwise.
     */
    bool remove_helper(TrieNode* current, const std::string& word, int depth) {
        if (!current) {
            return false;
        }

        if (depth == word.length()) {
            std::unique_lock<std::shared_mutex> lock(current->mtx);
            if (!current->isEndOfWord) {
                return false; // Word doesn't exist
            }
            current->isEndOfWord = false;
            return current->children.empty();
        }

        char ch = word[depth];
        std::unique_lock<std::shared_mutex> lock(current->mtx);
        if (current->children.find(ch) == current->children.end()) {
            return false; // Word doesn't exist
        }

        if (remove_helper(current->children[ch].get(), word, depth + 1)) {
            current->children.erase(ch);
            return !current->isEndOfWord && current->children.empty();
        }

        return false;
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

    // Demonstrate new features
    std::cout << "\n--- Demonstrating New Features ---" << std::endl;
    trie.insert("apple");
    trie.insert("app");
    trie.insert("application");
    trie.insert("apricot");

    std::cout << "Words with prefix 'app':" << std::endl;
    for (const auto& word : trie.words_with_prefix("app")) {
        std::cout << "  - " << word << std::endl;
    }

    std::cout << "\nRemoving 'apple'..." << std::endl;
    trie.remove("apple");

    std::cout << "Words with prefix 'app' after removal:" << std::endl;
    for (const auto& word : trie.words_with_prefix("app")) {
        std::cout << "  - " << word << std::endl;
    }

    return 0;
}
