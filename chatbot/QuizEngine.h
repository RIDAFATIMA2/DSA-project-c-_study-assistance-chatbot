#pragma once
#include <string>

class QuizEngine {
public:
    static bool testMode;  // flag to enable test/non-interactive mode
    QuizEngine(const std::string &dataBasePath);
    void runQuiz(const std::string &topic);
    
    // Structure to hold quiz result
    struct QuizResult {
        std::string topic;
        std::string difficulty;
        int score;
        int total;
    };
    
    // Save quiz result to user progress file
    void saveProgress(const std::string &username, const QuizResult &result);
    
    // Display user progress from file
    void displayProgress(const std::string &username) const;
    
private:
    std::string dataBasePath_;
    
    // Helper to select difficulty level interactively
    std::string selectDifficulty();
    
    // Helper to parse and run questions for specific difficulty
    QuizResult runQuizByDifficulty(const std::string &topic, const std::string &difficulty);
};