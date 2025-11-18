#pragma once
#include <string>

class DataManager {
public:
    DataManager(const std::string &basePath);
    std::string loadTopicContent(const std::string &topic);
    // Load a specific named section from a topic file (case-insensitive), e.g. "definition", "pseudocode", "example", "faq".
    // Returns empty string if section not found.
    std::string loadTopicSection(const std::string &topic, const std::string &section);
    void ensureUserFiles();
    std::string getBasePath() const;
private:
    std::string basePath_; // root path to data folder
};