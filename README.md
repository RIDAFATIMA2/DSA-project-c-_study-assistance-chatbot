# DSA-project-cpp_study-assistance-chatbot
A rule-based offline DSA chatbot built in C++ in my 3rd semester, before learning AI/ML/NLP. It offers topic explanations, pseudocode, comparisons, and adaptive quizzes using lightweight NLP, stacks for context, and append-only files for user progress.

Project Report: DSA Interactive Learning Chatbot
1. Introduction
The DSA Interactive Learning Chatbot is a console-based educational assistant developed in C++. The system is designed to help students learn Data Structures and Algorithms (DSA) through interactive conversations, structured topic explanations, adaptive quizzes, and persistent progress tracking. This chatbot aims to create a study experience that feels personalized and intelligent—without using external AI or machine-learning models. Instead, it relies fully on core programming concepts and data structures such as stacks, maps, vectors, and file-based storage. The project demonstrates how classical DSA principles can be used to simulate AI-like behavior, maintain conversational context, and create adaptive learning pathways for students.

2. Project Overview
The chatbot functions as a DSA tutor, allowing the user to: ask for definitions, explanations, comparisons, pseudocode, or examples; take quizzes with difficulty levels (Easy/Medium/Hard); continue conversations based on previous topics using stack-based context; preserve learning history across multiple sessions; retrieve progress and review past quizzes anytime. The overall design focuses on interactive learning, adaptive difficulty, and permanent user progress storage.

3. Key Features
1. Conversational Learning: The chatbot allows free-text interaction and recognizes commands such as: “teach me bst,” “explain linked list,” “pseudocode for inorder traversal,” “difference between stack and queue.” A lightweight NLP module identifies both intent (quiz, detail, learn, compare) and topic (array, BST, graph, heap, etc.).
2. Adaptive Quizzing: Each DSA topic contains three difficulty tiers: Easy, Medium, Hard. Every tier includes 5 curated questions stored in dedicated files (e.g., stack_quiz.txt, graph_quiz.txt). The quiz engine evaluates answers using flexible matching.
3. Context Continuity: A stack of topics keeps track of what the user is studying, enabling follow-up questions like “explain more,” “give me the pseudocode,” and “quiz me.”
4. Persistent Append-Only Progress Tracking: The chatbot stores all user data in two append-only files: quiz_progress.txt (username|topic:score/total:difficulty) and topics_history.txt (username|sessions:topic1,topic2,...). No entries are ever deleted.

4. System Architecture
The architecture uses modular C++ classes:
A. main.cpp: Initializes the app and starts the chatbot loop.
B. ChatEngine: Handles conversation, context stack, FAQ-first matching, NLP actions, and topic history.
C. QuizEngine: Loads quiz files, runs quizzes, normalizes answers, and saves results.
D. DataManager: Loads topic content, ensures required user files exist, migrates legacy data, and provides utilities.
E. NLP Module: Detects intent and topic using keyword matching, normalization, and fuzzy similarity.

5. Storage Design
The storage layer uses an append-only model.
Example quiz entry: alice|array:4/5:EASY
Example history entry: alice|sessions:array,stack,bst
Old progress.txt is automatically split into the new files on first run.

6. FAQ-First Matching Logic
When user input includes a known topic, the system loads FAQ items, normalizes text, computes similarity (longest common substring), and returns the FAQ answer if similarity ≥ 50%. Otherwise, it falls back to intent-based responses.

7. Usage Guide
Build: g++ -std=c++17 main.cpp NLP.cpp ChatEngine.cpp DataManager.cpp QuizEngine.cpp -o chatbot.exe
Run: .\chatbot.exe
Example commands: “teach me bst,” “quiz me on arrays,” “pseudocode for merge sort,” “compare array and linked list,” “show progress.”
Sample quiz interaction included in the original text.

8. Conclusion
The DSA Interactive Learning Chatbot demonstrates how classical DSA and clean C++ architecture can create an intelligent, interactive tutor without machine-learning frameworks. Using NLP-based intent detection, stack-driven context, adaptive quizzing, append-only storage, and structured explanations, the system provides a smooth learning experience and showcases strong programming, OOP design, file handling, and algorithmic thinking.
The DSA Interactive Learning Chatbot demonstrates how classical DSA and clean C++ architecture can create an intelligent, interactive tutor without machine-learning frameworks. Using NLP-based intent detection, stack-driven context, adaptive quizzing, append-only storage, and structured explanations, the system provides a smooth learning experience and showcases strong programming, OOP design, file handling, and algorithmic thinking.
