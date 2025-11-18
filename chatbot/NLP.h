#pragma once
#include <string>
#include <utility>

// Very small NLP-lite: extracts intent and topic from a user sentence.
// Returns pair<intent, topic>. If not found, returns "unknown".
std::pair<std::string, std::string> parseIntentAndTopic(const std::string &input);