
#include "ChatEngine.h"
#include "NLP.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <cctype>
#ifdef _WIN32
#include <windows.h>
#endif

ChatEngine::ChatEngine(DataManager &dm) : dm_(dm), quiz_(dm.getBasePath()) {}

static void setColor(int colorCode) {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, colorCode);
#endif
}

static void resetColor() {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, 7);
#endif
}

void ChatEngine::typePrint(const std::string &s, int msDelay) {
    // If string very large, print without typing effect
    if (s.size() > 1500) {
        setColor(11); // Cyan
        std::cout << s << std::endl;
        resetColor();
        return;
    }

    // Print with small delay to mimic typing
    setColor(11); // Cyan
    for (char c : s) {
        std::cout << c << std::flush;
#ifdef _WIN32
        Sleep(msDelay);
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(msDelay));
#endif
    }
    std::cout << std::endl;
    resetColor();
}

static std::string lower(const std::string &s) {
    std::string t = s;
    std::transform(t.begin(), t.end(), t.begin(), ::tolower);
    return t;
}

// Helper to normalize string for FAQ matching (remove spaces, punctuation)
static std::string normalizeFAQ(const std::string &s) {
    std::string result;
    for (char c : s) {
        if (std::isalnum(c)) {
            result += std::tolower(c);
        }
    }
    return result;
}

// Longest common substring length (used for fuzzy FAQ matching)
static int longestCommonSubstring(const std::string &a, const std::string &b) {
    if (a.empty() || b.empty()) return 0;
    int n = (int)a.size();
    int m = (int)b.size();
    std::vector<std::vector<int>> dp(n+1, std::vector<int>(m+1, 0));
    int best = 0;
    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            if (a[i-1] == b[j-1]) {
                dp[i][j] = dp[i-1][j-1] + 1;
                if (dp[i][j] > best) best = dp[i][j];
            }
        }
    }
    return best;
}

std::string ChatEngine::getContextTopic() {
    if (!topicStack_.empty()) {
        return topicStack_.top();
    }
    return "unknown";
}

void ChatEngine::saveSessionProgress() {
    if (sessionTopics_.empty()) return;
    
    std::cout << "\nWould you like to save your session progress? (yes/no): ";
    std::string saveChoice;
    std::getline(std::cin, saveChoice);
    
    if (saveChoice == "yes" || saveChoice == "y" || saveChoice == "sure" || saveChoice == "yep") {
        std::cout << "Enter your username: ";
        std::string username;
        std::getline(std::cin, username);
        
        std::string userPath = dm_.getBasePath() + "\\user\\topics_history.txt";

        // Append a new session line for the user. We intentionally append to preserve history.
        std::ofstream outFile(userPath, std::ios::app);
        if (!outFile) {
            std::cout << "Unable to open topics history file for writing: " << userPath << "\n";
            return;
        }

        outFile << username << "|sessions:";
        for (size_t i = 0; i < sessionTopics_.size(); ++i) {
            if (i > 0) outFile << ",";
            outFile << sessionTopics_[i];
        }
        outFile << "\n";
        outFile.close();
        
        setColor(2); // Green
        std::cout << "Session progress saved!\n";
        resetColor();
    }
}

void ChatEngine::start() {
    typePrint("Hi! I'm your DSA study assistant.");
    typePrint("What do you want to study today? (e.g., bst, queue, linked_list, graphs, binary_tree, sorting...)");

    while (true) {
        std::cout << "\nYou: ";
        setColor(7); // White
        std::string input;
        std::getline(std::cin, input);
        resetColor();
        
        if (!std::cin) break;

        // Normalize input early
        input = lower(input);

        // Exit synonyms
        if (input == "exit" || input == "quit" || input == "bye" || input == "stop"|| input == "end" || input == "end session") {
            saveSessionProgress();
            typePrint("Bye! Keep practicing.");
            // Empty the stack
            while (!topicStack_.empty()) topicStack_.pop();
            break;
        }
        
        // Check for progress display request
        if (input.find("progress") != std::string::npos || input.find("show progress") != std::string::npos) {
            std::cout << "Enter your username to view progress: ";
            std::string username;
            std::getline(std::cin, username);
            quiz_.displayProgress(username);
            continue;
        }

        auto parsed = parseIntentAndTopic(input);
        std::string intent = parsed.first;
        std::string topic = parsed.second;

        // If multiple topics were detected (joined with '|'), pick the most specific one
        auto chooseBestTopic = [&](const std::string &topicStr, const std::string &origInput) {
            if (topicStr.find('|') == std::string::npos) return topicStr;
            // split
            std::vector<std::string> parts;
            std::istringstream pss(topicStr);
            std::string seg;
            while (std::getline(pss, seg, '|')) if (!seg.empty()) parts.push_back(seg);

            // If the original input explicitly mentions a short name, prefer that
            for (auto &t : parts) {
                // The stored topic keys use underscores (e.g., "linked_list") while user input
                // typically uses spaces ("linked list"). Check both forms when matching.
                if (origInput.find(t) != std::string::npos) return t;
                std::string t_space = t;
                for (auto &c : t_space) if (c == '_') c = ' ';
                if (origInput.find(t_space) != std::string::npos) return t;
            }

            // Prefer more specific topics if present
            if (std::find(parts.begin(), parts.end(), std::string("avl_tree")) != parts.end()) return std::string("avl_tree");
            if (std::find(parts.begin(), parts.end(), std::string("bst")) != parts.end()) return std::string("bst");
            if (std::find(parts.begin(), parts.end(), std::string("binary_tree")) != parts.end()) return std::string("binary_tree");

            // default to first
            return parts.empty() ? std::string("unknown") : parts[0];
        };

        // pick best topic now
        std::string chosenTopic = (topic == "unknown") ? topic : chooseBestTopic(topic, input);
        topic = chosenTopic;

        
        // If topic not detected but context exists, use top of stack
        if (topic == "unknown" && !topicStack_.empty()) {
            topic = topicStack_.top();
            // For ambiguous requests like "explain in detail", use context
            if (intent == "unknown" || intent == "detail") {
                intent = "detail";
            }
        }

        if (intent == "unknown" && topic == "unknown") {
            typePrint("Hmm... I didn't quite understand. Try something like:");
            typePrint("  teach me bst");
            typePrint("  quiz me on queues");
            typePrint("  give example of stack");
            typePrint("  difference between array and linked list");
            continue;
        }

        // Push detected topic to stack if new
        if (topic != "unknown") {
            std::string mainTopic = topic;
            size_t sep = mainTopic.find('|');
            if (sep != std::string::npos) mainTopic = mainTopic.substr(0, sep);
            
            // Only push if different from top
            if (topicStack_.empty() || topicStack_.top() != mainTopic) {
                topicStack_.push(mainTopic);
                // Add to session topics if not already there
                if (std::find(sessionTopics_.begin(), sessionTopics_.end(), mainTopic) == sessionTopics_.end()) {
                    sessionTopics_.push_back(mainTopic);
                }
            }
        }

        if (intent == "learn" || intent == "definition") {
            if (topic == "unknown") {
                typePrint("Sure, which topic? (bst, queue, linked_list, array, binary_tree, sorting...)");
                continue;
            }

            std::string mainTopic = topic;
            size_t sep = mainTopic.find('|');
            if (sep != std::string::npos) mainTopic = mainTopic.substr(0, sep);

            std::string section = dm_.loadTopicSection(mainTopic, "definition");
            if (section.empty()) section = dm_.loadTopicContent(mainTopic);

            // DEBUG: 
            // std::cerr << "[DEBUG] learn block: intent=" << intent << ", topic=" << mainTopic << ", section_len=" << section.length() << "\n";

            // Show only a concise definition (first paragraph)
            std::istringstream ss(section);
            std::string line, out;
            while (std::getline(ss, line)) {
                if (!line.empty()) { out = line; break; }
            }
            if (out.empty()) out = section;
            typePrint(out, 1);
            typePrint("Do you want more detail, pseudocode, example, or a quiz?");
            continue;
        }

        if (intent == "detail") {
            if (topic == "unknown") {
                typePrint("Which topic would you like more detail on?");
                continue;
            }
            
            std::string mainTopic = topic;
            size_t sep = mainTopic.find('|');
            if (sep != std::string::npos) mainTopic = mainTopic.substr(0, sep);
            
            // Output complete topic file
            std::string content = dm_.loadTopicContent(mainTopic);
            typePrint(content, 1);
            continue;
        }

        if (intent == "quiz") {
            if (topic == "unknown") {
                typePrint("Which topic should I quiz you on?");
                continue;
            }
            std::string mainTopic = topic;
            size_t sep = mainTopic.find('|');
            if (sep != std::string::npos) mainTopic = mainTopic.substr(0, sep);
            quiz_.runQuiz(mainTopic);
            continue;
        }

        if (intent == "example" || intent == "pseudocode") {
            if (topic == "unknown") {
                typePrint("Which topic are you asking about?");
                continue;
            }

            std::string mainTopic = topic;
            size_t sep = mainTopic.find('|');
            if (sep != std::string::npos) mainTopic = mainTopic.substr(0, sep);

            std::string section = dm_.loadTopicSection(mainTopic, intent == "example" ? "example" : "pseudocode");
            if (section.empty()) {
                typePrint("I couldn't find a specific section. Here's a summary:");
                std::string sum = dm_.loadTopicSection(mainTopic, "definition");
                if (sum.empty()) sum = dm_.loadTopicContent(mainTopic);
                typePrint(sum, 1);
            } else {
                // If user asked for specific pseudocode (e.g., preorder/inorder), try to extract only that part
                if (intent == "pseudocode") {
                    std::vector<std::string> subkeys = {"preorder", "inorder", "postorder", "search", "insert", "delete", "traversal"};
                    std::string foundSub;
                    for (auto &k : subkeys) if (input.find(k) != std::string::npos) { foundSub = k; break; }

                    if (!foundSub.empty()) {
                        // Look through the full topic content for labeled pseudocode blocks and return
                        // the single block whose heading or body contains the requested subkey.
                        std::string full = dm_.loadTopicContent(mainTopic);
                        std::istringstream css(full);
                        std::string line;
                        std::string curHeading;
                        std::string curBody;
                        std::vector<std::pair<std::string,std::string>> blocks;
                        while (std::getline(css, line)) {
                            if (!line.empty() && line.back() == '\r') line.pop_back();
                            std::string low = lower(line);
                            if (low.find("pseudocode:") == 0) {
                                if (!curHeading.empty()) {
                                    blocks.push_back({curHeading, curBody});
                                    curBody.clear();
                                }
                                // heading text after label
                                size_t col = line.find(':');
                                curHeading = (col==std::string::npos) ? line : line.substr(col+1);
                                // trim
                                size_t st = curHeading.find_first_not_of(" \t");
                                if (st!=std::string::npos) curHeading = curHeading.substr(st);
                            } else if (!curHeading.empty()) {
                                if (line.empty()) {
                                    // end of this pseudocode block
                                    blocks.push_back({curHeading, curBody});
                                    curHeading.clear(); curBody.clear();
                                } else {
                                    if (!curBody.empty()) curBody += "\n";
                                    curBody += line;
                                }
                            }
                        }
                        if (!curHeading.empty()) blocks.push_back({curHeading, curBody});

                        bool printed = false;
                        for (auto &b : blocks) {
                            std::string lowh = lower(b.first);
                            std::string lowb = lower(b.second);
                            if (lowh.find(foundSub) != std::string::npos || lowb.find(foundSub) != std::string::npos) {
                                std::string out = b.first + "\n" + b.second;
                                size_t st = out.find_first_not_of("\n \t");
                                if (st!=std::string::npos) out = out.substr(st);
                                typePrint(out, 1);
                                printed = true;
                                break;
                            }
                        }
                        if (!printed) {
                            // fallback to the original section if no specific block found
                            typePrint(section, 1);
                        }
                    } else {
                        typePrint(section, 1);
                    }
                } else {
                    typePrint(section, 1);
                }
            }
            continue;
        }

        if (intent == "difference") {
            if (topic == "unknown") {
                typePrint("Which two topics do you want to compare? e.g. 'difference between array and linked list'");
                continue;
            }
            std::string t1 = topic, t2;
            size_t sep = t1.find('|');
            if (sep != std::string::npos) {
                t2 = t1.substr(sep+1);
                t1 = t1.substr(0, sep);
            }
            if (t2.empty()) {
                // Fallback: try splitting the original input around ' and ' or ' vs ' to detect two topics
                size_t andPos = input.find(" and ");
                size_t vsPos = input.find(" vs ");
                size_t splitPos = std::string::npos;
                if (andPos != std::string::npos) splitPos = andPos;
                else if (vsPos != std::string::npos) splitPos = vsPos;

                if (splitPos != std::string::npos) {
                    std::string left = input.substr(0, splitPos);
                    std::string right = input.substr(splitPos + ((andPos!=std::string::npos) ? 5 : 4));
                    auto lpair = parseIntentAndTopic(left);
                    auto rpair = parseIntentAndTopic(right);
                    if (lpair.second != "unknown" && rpair.second != "unknown") {
                        t1 = lpair.second;
                        t2 = rpair.second;
                    }
                }

                if (t2.empty()) {
                    typePrint("Please mention two topics to compare (e.g., 'array and linked list').");
                    continue;
                }
            }

            std::string def1 = dm_.loadTopicSection(t1, "definition");
            std::string def2 = dm_.loadTopicSection(t2, "definition");
            if (def1.empty()) def1 = dm_.loadTopicContent(t1);
            if (def2.empty()) def2 = dm_.loadTopicContent(t2);

            auto firstNonEmpty = [](const std::string &s){
                std::istringstream ss(s); std::string l; while (std::getline(ss,l)) if (!l.empty()) return l; return std::string(); };
            std::string c1 = firstNonEmpty(def1);
            std::string c2 = firstNonEmpty(def2);

            typePrint(t1 + ": " + (c1.empty() ? "(no short definition)" : c1));
            typePrint(t2 + ": " + (c2.empty() ? "(no short definition)" : c2));
            typePrint("Short difference:");
            
            if ((t1=="array" && t2=="linked_list") || (t2=="array" && t1=="linked_list")) {
                typePrint("Arrays use contiguous memory and offer O(1) index access; linked lists use dynamic nodes and allow O(1) insert/delete at head but O(n) access.");
            } else if ((t1=="stack" && t2=="queue") || (t2=="stack" && t1=="queue")) {
                typePrint("Stacks follow LIFO; queues follow FIFO. Stack removes from top; queue removes from front.");
            } else if ((t1=="bst" && t2=="binary_tree") || (t2=="bst" && t1=="binary_tree")) {
                typePrint("A BST is a binary tree with ordering property (left < node < right). Not all binary trees are BSTs.");
            } else if ((t1=="bst" && t2=="avl_tree") || (t2=="bst" && t1=="avl_tree")) {
                typePrint("AVL is a self-balancing BST. Both follow BST rules but AVL maintains strict height balance for guaranteed O(log n) operations.");
            } else if ((t1=="queue" && t2=="deque") || (t2=="queue" && t1=="deque")) {
                typePrint("A queue inserts at rear and deletes at front; a deque allows insert/delete at both ends.");
            } else if ((t1=="hash_table" && t2=="binary_search") || (t2=="hash_table" && t1=="binary_search")) {
                typePrint("Hash table average search is O(1); binary search is O(log n) but requires sorted array.");
            } else if ((t1=="array" && t2=="vector") || (t2=="array" && t1=="vector")) {
                typePrint("Arrays are fixed-size; vectors are dynamic arrays that resize automatically.");
            } else if ((t1=="vector" && t2=="linked_list") || (t2=="vector" && t1=="linked_list")) {
                typePrint("Vectors give O(1) random access; linked lists give O(1) insert/delete but O(n) access.");
            } else if ((t1=="graph" && t2=="tree") || (t2=="graph" && t1=="tree")) {
                typePrint("A tree is a special graph with no cycles and a single root; graphs may have cycles and no root.");
            } else if ((t1=="dfs" && t2=="bfs") || (t2=="dfs" && t1=="bfs")) {
                typePrint("DFS goes deep first using a stack; BFS explores level-wise using a queue.");
            } else if ((t1=="heap" && t2=="bst") || (t2=="heap" && t1=="bst")) {
                typePrint("A heap maintains heap-order (parent > children or vice versa); a BST keeps keys sorted based on left < root < right.");
            } else {
                typePrint("(If you want a detailed comparison, ask for more details of both topics one by one,as i dont have one liner difference for it.)");
            }
            
            continue;
        }

         // Immediately attempt FAQ-first matching for detected topic.
        if (topic != "unknown") {
            std::string mainTopic = topic;
            size_t sep = mainTopic.find('|');
            if (sep != std::string::npos) mainTopic = mainTopic.substr(0, sep);

            std::string faqSection = dm_.loadTopicSection(mainTopic, "FAQ");
            if (!faqSection.empty()) {
                std::string normalizedInput = normalizeFAQ(input);

                std::istringstream ss(faqSection);
                std::string line;
                std::string curQ, curA;
                std::string bestAnswer;
                int bestScore = 0;

                while (std::getline(ss, line)) {
                    if (line.rfind("Q:", 0) == 0) {
                        curQ = line.substr(2);
                    } else if (line.rfind("A:", 0) == 0) {
                        curA = line.substr(2);
                        if (!curQ.empty()) {
                            std::string normalizedQ = normalizeFAQ(curQ);
                            int lcs = longestCommonSubstring(normalizedInput, normalizedQ);

                            // If input or question contains the other, treat as strong match
                            if (!normalizedQ.empty() && (normalizedQ.find(normalizedInput) != std::string::npos || normalizedInput.find(normalizedQ) != std::string::npos)) {
                                lcs = std::max((int)normalizedInput.size(), (int)normalizedQ.size());
                            }

                            // If at least half of either string matches, consider it good
                            int threshold = std::max(1, std::min((int)normalizedQ.size()/2, (int)normalizedInput.size()/2));
                            if (lcs >= threshold && lcs > bestScore) {
                                bestScore = lcs;
                                bestAnswer = curA;
                            }
                        }
                        curQ.clear(); curA.clear();
                    }
                }

                if (bestScore > 0 && !bestAnswer.empty()) {
                    typePrint(bestAnswer, 1);
                    continue;
                }
            }
        }
        

        typePrint("Sorry, I couldn't handle that request yet. Try 'explain <topic>' or 'quiz <topic>'.");
    }
}
