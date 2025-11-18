#include <iostream>
#include "DataManager.h"
#include "ChatEngine.h"
#include "QuizEngine.h"


int main(int argc, char* argv[]) {
    // Check for --test-mode flag
    bool testMode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--test-mode") {
            testMode = true;
            break;
        }
    }
    
    QuizEngine::testMode = testMode;
    
    // base data path is ./data
    std::string dataPath = "data";
    DataManager dm(dataPath);
    ChatEngine chat(dm);
    chat.start();
    return 0;
}
