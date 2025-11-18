#pragma once
#include "DataManager.h"
#include "QuizEngine.h"
#include <string>
#include <stack>
#include <vector>

class ChatEngine {
public:
    ChatEngine(DataManager &dm);
    void start();
private:
    DataManager &dm_;
    QuizEngine quiz_;
    std::stack<std::string> topicStack_;  // Store context of discussed topics
    std::vector<std::string> sessionTopics_;  // Track all topics in session
    
    void typePrint(const std::string &s, int msDelay = 4);
    std::string getContextTopic();  // Get current topic from stack or return unknown
    void saveSessionProgress();  // Save session data to user.txt
};