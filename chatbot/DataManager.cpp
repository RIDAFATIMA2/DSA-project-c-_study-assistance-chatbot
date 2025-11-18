#include "DataManager.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

DataManager::DataManager(const std::string &basePath) : basePath_(basePath) {
    ensureUserFiles();
}

std::string DataManager::loadTopicContent(const std::string &topic) {
    std::string path = basePath_ + "\\topics\\" + topic + ".txt";
    std::ifstream in(path);
    if (!in) return "[No content available for this topic yet.]";
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Case-insensitive search for a section heading (e.g. "Definition:") and return its body.
std::string DataManager::loadTopicSection(const std::string &topic, const std::string &section) {
    std::string content = loadTopicContent(topic);
    if (content.rfind("[No content available", 0) == 0) return std::string();
    // Split content into lines and find a heading that matches the requested section (case-insensitive)
    std::istringstream ss(content);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ss, line)) {
        // trim CR
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }

    std::string lowerSection = section;
    for (auto &c : lowerSection) c = (char)std::tolower((unsigned char)c);

    for (size_t i = 0; i < lines.size(); ++i) {
        std::string lowerLine = lines[i];
        for (auto &c : lowerLine) c = (char)std::tolower((unsigned char)c);

        // match heading like "Definition:" at the beginning, or exactly the word (case-insensitive)
        if (lowerLine.find(lowerSection + ":") == 0 || lowerLine == lowerSection) {
            // collect the paragraph immediately following the heading (until the next blank line)
            std::string out;
            size_t j = i + 1;
            while (j < lines.size()) {
                if (lines[j].empty()) break; // paragraph ended
                if (!out.empty()) out += "\n";
                out += lines[j];
                ++j;
            }

            // trim leading/trailing whitespace
            size_t startTrim = out.find_first_not_of("\n \t");
            if (startTrim == std::string::npos) return std::string();
            size_t endTrim = out.find_last_not_of("\n \t");
            return out.substr(startTrim, endTrim - startTrim + 1);
        }
    }

    // Try common alternate heading names (fallback)
    // Only match these as headings if they are at the beginning of the line (after trimming) or followed by colon
    std::vector<std::string> alternates = {"definition","introduction","intro","representation","types","overview"};
    for (auto &alt : alternates) {
        std::string lowAlt = alt;
        for (auto &c : lowAlt) c = (char)std::tolower((unsigned char)c);
        for (size_t i = 0; i < lines.size(); ++i) {
            std::string lowerLine = lines[i];
            for (auto &c : lowerLine) c = (char)std::tolower((unsigned char)c);
            // Only match if it's a heading with colon, or the entire line is just the word
            if (lowerLine.find(lowAlt + ":") == 0 || lowerLine == lowAlt) {
                std::string out;
                size_t j = i + 1;
                while (j < lines.size()) {
                    if (lines[j].empty()) break;
                    if (!out.empty()) out += "\n";
                    out += lines[j];
                    ++j;
                }
                size_t startTrim = out.find_first_not_of("\n \t");
                if (startTrim == std::string::npos) return std::string();
                size_t endTrim = out.find_last_not_of("\n \t");
                return out.substr(startTrim, endTrim - startTrim + 1);
            }
        }
    }

    return std::string();
}

void DataManager::ensureUserFiles() {
    // Ensure user directory exists and initialize progress.txt
    std::string userDir = basePath_ + "\\user";
    std::string profilePath = userDir + "\\profile.txt";
    std::string progressPath = userDir + "\\progress.txt";
    std::string quizPath = userDir + "\\quiz_progress.txt";
    std::string topicsPath = userDir + "\\topics_history.txt";

    std::ifstream checkProfile(profilePath);
    if (!checkProfile) {
        std::ofstream outProfile(profilePath);
        outProfile << "username:guest\nquizzes_taken:0\n";
        outProfile.close();
    }

    // Migrate legacy progress.txt if present into two separate files:
    // - quiz_progress.txt : stores one quiz entry per line (username|topic:score/total:difficulty)
    // - topics_history.txt : stores one session entry per line (username|sessions:topic1,topic2,...)
    std::ifstream oldProgress(progressPath);
    if (oldProgress) {
        std::vector<std::string> quizLines;
        std::vector<std::string> topicLines;
        std::string line;
        while (std::getline(oldProgress, line)) {
            if (line.empty()) continue;
            if (line.rfind("#", 0) == 0) continue;
            size_t pipe = line.find('|');
            if (pipe == std::string::npos) continue;
            std::string right = line.substr(pipe + 1);
            if (right.rfind("sessions:", 0) == 0) {
                // Keep the whole sessions line as-is
                topicLines.push_back(line);
            } else {
                // quizzes may be comma-separated; split them to one-per-line
                std::istringstream ss(right);
                std::string quiz;
                while (std::getline(ss, quiz, ',')) {
                    if (quiz.empty()) continue;
                    std::string outLine = line.substr(0, pipe + 1) + quiz;
                    quizLines.push_back(outLine);
                }
            }
        }
        oldProgress.close();

        if (!quizLines.empty()) {
            std::ofstream outQuiz(quizPath, std::ios::app);
            for (auto &l : quizLines) outQuiz << l << "\n";
        }
        if (!topicLines.empty()) {
            std::ofstream outTopics(topicsPath, std::ios::app);
            for (auto &l : topicLines) outTopics << l << "\n";
        }
    }

    // Ensure quiz_progress.txt exists
    std::ifstream checkQuiz(quizPath);
    if (!checkQuiz) {
        std::ofstream outQuiz(quizPath);
        outQuiz << "# Quiz progress file\n# Format: username|topic:score/total:difficulty\n";
        outQuiz.close();
    }

    // Ensure topics_history.txt exists
    std::ifstream checkTopics(topicsPath);
    if (!checkTopics) {
        std::ofstream outTopics(topicsPath);
        outTopics << "# Topics history file\n# Format: username|sessions:topic1,topic2,...\n";
        outTopics.close();
    }
}

std::string DataManager::getBasePath() const {
    return basePath_;
}