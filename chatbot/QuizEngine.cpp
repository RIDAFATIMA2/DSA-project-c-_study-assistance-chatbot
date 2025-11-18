#include "QuizEngine.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <ctime>
#ifdef _WIN32
#include <windows.h>
#endif
#include <set>

bool QuizEngine::testMode = false;

QuizEngine::QuizEngine(const std::string &dataBasePath) : dataBasePath_(dataBasePath) {}

static std::string normalize(const std::string &s) {
    std::string r;
    for (char c : s) if (!isspace((unsigned char)c)) r += std::tolower((unsigned char)c);
    return r;
}

static void setColor(int colorCode) {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, colorCode);
#endif
}

static void resetColor() {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, 7); // Default white
#endif
}

std::string QuizEngine::selectDifficulty() {
    if (testMode) {
        // In test mode, read difficulty from stdin without prompts
        std::string choice;
        std::getline(std::cin, choice);
        if (choice == "1" || choice == "easy") return "EASY";
        if (choice == "2" || choice == "medium") return "MEDIUM";
        if (choice == "3" || choice == "hard") return "HARD";
        return "EASY";  // default silently
    }
    
    std::cout << "\n";
    setColor(14); // Yellow
    std::cout << "Select difficulty level:\n";
    std::cout << "1. EASY (basic questions)\n";
    std::cout << "2. MEDIUM (Intermediate complexity)\n";
    std::cout << "3. HARD (Advanced questions)\n";
    resetColor();
    std::cout << "Enter choice (1-3): ";
    
    std::string choice;
    std::getline(std::cin, choice);
    
    if (choice == "1" || choice == "easy") return "EASY";
    if (choice == "2" || choice == "medium") return "MEDIUM";
    if (choice == "3" || choice == "hard") return "HARD";
    
    std::cout << "Invalid choice. Defaulting to EASY.\n";
    return "EASY";
}

QuizEngine::QuizResult QuizEngine::runQuizByDifficulty(const std::string &topic, const std::string &difficulty) {
    struct QA { std::string q; std::string a; };
    std::vector<QA> qa;
    bool inDifficultySection = false;
    
    // Load quiz file
    std::string path = dataBasePath_ + "\\topics\\" + topic + "_quiz.txt";
    std::ifstream in(path);
    
    if (in) {
        std::string line;
        std::string curQ, curA;
        
        while (std::getline(in, line)) {
            // Trim CR
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            
            // Check for difficulty markers
            if (line == "[EASY]") {
                inDifficultySection = (difficulty == "EASY");
                continue;
            } else if (line == "[MEDIUM]") {
                inDifficultySection = (difficulty == "MEDIUM");
                continue;
            } else if (line == "[HARD]") {
                inDifficultySection = (difficulty == "HARD");
                continue;
            }
            
            if (!inDifficultySection) continue;
            
            if (line.empty()) continue;
            
            if (line.rfind("Q:", 0) == 0) {
                curQ = line.substr(2);
            } else if (line.rfind("A:", 0) == 0) {
                curA = line.substr(2);
                if (!curQ.empty()) {
                    qa.push_back({curQ, curA});
                    curQ.clear(); curA.clear();
                }
            }
        }
        in.close();
    }
    
    if (qa.empty()) {
        std::cout << "No quiz found. Loading default questions...\n";
        qa = {
            {"What is a data structure?", "a way to organize data"},
            {"Name a linear data structure.", "array"},
            {"What does LIFO stand for?", "last in first out"},
            {"What does FIFO stand for?", "first in first out"},
            {"Define algorithm", "step by step procedure"}
        };
    }
    
    // Limit to 5 questions
    if (qa.size() > 5) qa.resize(5);
    
    int correct = 0;
    setColor(11); // Cyan
    std::cout << "\n========== " << difficulty << " Quiz on " << topic << " ==========\n";
    resetColor();
    
    for (size_t i = 0; i < qa.size(); ++i) {
        setColor(10); // Green
        std::cout << "\nQ" << (i+1) << ": ";
        resetColor();
        std::cout << qa[i].q << "\n> ";
        
        std::string ans;
        std::getline(std::cin, ans);
        
        // Normalize and check answer (flexible matching)
        std::string normalizedAns = normalize(ans);
        std::string normalizedCorrect = normalize(qa[i].a);
        // User wants to exit the quiz early
        std::string low = normalizedAns;
        if (low == "exit" || low == "quit" || low == "end" || low == "finish" ||
            low == "end session" || low == "stop") {

            setColor(14); // Yellow
            std::cout << "\nExiting quiz early...\n";
            resetColor();
            break; // immediately stop the quiz loop
        }
        if (normalizedAns.find(normalizedCorrect) != std::string::npos || 
            normalizedCorrect.find(normalizedAns) != std::string::npos) {
            setColor(2); // Green
            std::cout << " Correct!\n";
            resetColor();
            ++correct;
        } else {
            setColor(4); // Red
            std::cout << " Wrong! ";
            resetColor();
            std::cout << "\nCorrect answer: " << qa[i].a << "\n";
        }
    }
    
    setColor(11); // Cyan
    std::cout << "\n===================================\n";
    std::cout << "Quiz Score: " << correct << " / " << qa.size() << "\n";
    resetColor();
    
    return {topic, difficulty, correct, (int)qa.size()};
}

void QuizEngine::runQuiz(const std::string &topic) {
    std::string difficulty = selectDifficulty();
    QuizResult result = runQuizByDifficulty(topic, difficulty);
    
    // Ask to save progress
    std::cout << "\nWould you like to save your progress?  ";
    std::string saveChoice;
    std::getline(std::cin, saveChoice);
    
    if (saveChoice == "yes" || saveChoice == "y" || saveChoice == "sure" || saveChoice == "yep") {
        std::cout << "Enter your username: ";
        std::string username;
        std::getline(std::cin, username);
        saveProgress(username, result);
    }
}

void QuizEngine::saveProgress(const std::string &username, const QuizResult &result) {
    std::string userPath = dataBasePath_ + "\\user\\quiz_progress.txt";

    std::ofstream outFile(userPath, std::ios::app);
    if (!outFile) {
        std::cout << "Unable to open progress file for writing: " << userPath << "\n";
        return;
    }

    // Write one entry per line. Appending preserves history and does not remove older records.
    // Format: username|topic:score/total:difficulty
    outFile << username << "|" << result.topic << ":" << result.score << "/" << result.total << ":" << result.difficulty << "\n";
    outFile.close();

    setColor(2); // Green
    std::cout << "Progress saved successfully!\n";
    resetColor();
}

void QuizEngine::displayProgress(const std::string &username) const {
    std::string userPath = dataBasePath_ + "\\user\\quiz_progress.txt";
    std::ifstream inFile(userPath);

    if (!inFile) {
        std::cout << "No progress file found.\n";
        return;
    }

    std::string line;
    int entryNum = 0;
    setColor(11); // Cyan
    std::cout << "\n========== Progress for " << username << " ==========" << "\n";
    resetColor();

    while (std::getline(inFile, line)) {
        if (line.rfind(username + "|", 0) == 0) {
            // parse right side: topic:score/total:difficulty
            size_t pipePos = line.find('|');
            if (pipePos == std::string::npos) continue;
            std::string right = line.substr(pipePos + 1);
            std::istringstream qss(right);
            std::string topic, scoreStr, difficulty;
            std::getline(qss, topic, ':');
            std::getline(qss, scoreStr, ':');
            std::getline(qss, difficulty, ':');

            entryNum++;
            setColor(10); // Green
            std::cout << "Entry " << entryNum << ": ";
            resetColor();
            std::cout << topic << " (" << difficulty << ") - Score: " << scoreStr << "\n";
        }
    }

    if (entryNum == 0) {
        std::cout << "No progress found for user: " << username << "\n";
    } else {
        setColor(11); // Cyan
        std::cout << "=====================================\n";
        resetColor();
    }
    inFile.close();

    // Now show topics/sessions from topics_history.txt
    std::string topicsPath = dataBasePath_ + "\\user\\topics_history.txt";
    std::ifstream tFile(topicsPath);
    if (tFile) {
        std::set<std::string> studied;
        while (std::getline(tFile, line)) {
            if (line.rfind(username + "|sessions:", 0) == 0) {
                size_t pos = line.find('|');
                if (pos == std::string::npos) continue;
                std::string right = line.substr(pos + 1);
                // right starts with sessions:
                size_t colon = right.find(':');
                if (colon == std::string::npos) continue;
                std::string list = right.substr(colon + 1);
                std::istringstream lss(list);
                std::string topic;
                while (std::getline(lss, topic, ',')) {
                    if (!topic.empty()) studied.insert(topic);
                }
            }
        }
        if (!studied.empty()) {
            setColor(11); // Cyan
            std::cout << "\nTopics studied by " << username << ":\n";
            resetColor();
            for (auto &t : studied) std::cout << " - " << t << "\n";
        }
        tFile.close();
    }
}
