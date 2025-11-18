#include "NLP.h"
#include <algorithm>
#include <cctype>
#include <vector>

static std::string lowercase(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
    return s;
}

std::pair<std::string, std::string> parseIntentAndTopic(const std::string &input) {
    std::string s = lowercase(input);

    // Intents and synonyms
    // More granular intents: definition, pseudocode, example, quiz, compare/difference, progress, exit
    std::vector<std::pair<std::string, std::vector<std::string>>> intents = {
        {"pseudocode", {"pseudocode", "pseudo code", "pseudo-code", "pseudocode for", "pseudocode of", "algorithm", "implementation", "code", "algo", "how to implement", "how to code", "show code", "implementation of", "steps", "procedure", "how does", "how to"}},
        {"difference", {"difference", "different", "vs", "versus", "vs.", "differ", "compare", "comparision", "comparison", "between", "fark"}},
        {"learn", {"learn", "teach me", "learning", "tell me about", "explain", "know", "what is", "what are", "kiya hai", "kiya hota hai", "kiya"}},
        {"detail", {"detail", "detailed", "in depth", "in-depth", "more info", "more information", "explaination", "tell me more", "elaborate", "explain in detail", "go deeper"}},
        {"example", {"example", "eg", "sample", "demo", "illustration", "show example", "give example"}},
        {"definition", {"what is", "what are", "define", "definition", "kya hai", "meaning of", "show", "show me", "give", "give me", "tell me", "display"}},
        {"quiz", {"quiz", "test", "practice", "question", "questions", "qs"}},
        {"progress", {"progress", "history", "score", "performance"}},
        {"exit", {"exit", "quit", "bye", "goodbye", "end session", "close chat", "finish", "end"}}
    };

    std::vector<std::pair<std::string, std::vector<std::string>>> topics = {
        {"intro", {"introduction to dsa", "introduction", "intro", "abstract data types", "what is dsa", "dsa", "what is dsa"}},
        {"doubly_circular_linked_list", {"doubly linked list", "circular linked list", "doubly", "circular linked", "doubly linked", "doubly-linked"}},
        {"array", {"array", "arrays", "linear search", "binary search", "binary-search"}},
        {"avl_tree", {"avl", "avl tree", "balanced bst", "avl-tree"}},
        {"linked_list", {"linked list", "linked-list", "link list", "singly linked list", "linkedlist", "linkedlists"}},
        {"recursion_stack", {"recursion", "call stack", "call-stack"}},
        {"heap", {"heap","heap sort","heapify", "binary heap", "binary-heap"}},
        {"stack", {"stack", "stacks", "stack (adt)", "lifo"}},
        {"binary_heap", {"binary heap", "heap", "heap sort", "binary-heap", "heap sort", "priority queue", "priority-queue"}},
        {"deque", {"deque", "double ended queue", "double-ended queue", "dequeue"}},
        {"queue", {"queue", "queues", "circular queue", "deque", "double ended queue", "double-ended queue"}},
        {"sorting_elementary", {"bubble sort", "insertion sort", "selection sort", "sorting elementary", "elementary sorting", "sorting"}},
        {"searching", {"search", "searching", "binary search", "linear search", "binary-search"}},
        {"complexity", {"complexity", "time complexity", "space complexity", "big-o", "big o"}},
        {"sorting_advanced", {"merge sort", "quick sort", "quick-sort", "advanced sorting", "sorting advanced", "heap sort", "heapsort"}},
        {"binary_tree", {"binary tree", "binary-tree", "tree properties", "preorder", "inorder", "postorder", "traversal", "tree traversal"}},
        {"tree", {"tree", "trees", "tree data structure", "tree structure", "tree definition"}},
        {"bst", {"bst", "binary search tree", "binary-search-tree"}},
        {"binary_search", {"binary search", "binary-search"}},
        {"vector", {"vector", "vectors", "std::vector", "c++ vector"}},
        {"graph_advanced", {"shortest path", "mst", "minimum spanning", "topological sort", "dijkstra", "kruskal", "prim", "topological", "advanced graph"}},
        {"graph_basic", {"graph", "graphs", "graph representation", "graph traversal", "bfs", "dfs", "breadth first", "depth first"}},
        {"hashing", {"hashing", "hash function", "collision", "rehash", "hash table", "hash-table", "hash_table"}}
    };

    std::string foundIntent = "unknown";
    // prefer more specific intents (order above matters)
    for (auto &p : intents) {
        for (auto &kw : p.second) {
            if (s.find(kw) != std::string::npos) {
                foundIntent = p.first;
                break;
            }
        }
        if (foundIntent != "unknown") break;
    }

    // Collect up to two topics (for comparisons like "difference between array and linked list")
    std::vector<std::string> foundTopics;
    for (auto &p : topics) {
        for (auto &kw : p.second) {
            if (s.find(kw) != std::string::npos) {
                foundTopics.push_back(p.first);
                break;
            }
        }
        if (foundTopics.size() >= 2) break;
    }

    std::string foundTopic = "unknown";
    if (!foundTopics.empty()) {
        // join multiple topics with '|'
        foundTopic = foundTopics[0];
        for (size_t i = 1; i < foundTopics.size(); ++i) foundTopic += std::string("|") + foundTopics[i];
    }

    // If no explicit intent but a topic exists, assume user wants a definition/learn
    if (foundIntent == "unknown") {
        if (!foundTopics.empty()) foundIntent = "definition";
        else foundIntent = "unknown";
    }

    return {foundIntent, foundTopic};
}