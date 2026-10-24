//============================================================================
// Name        : Mia Slevinsky
// Description : ABCU Advising Assistance Program - Binary Search Tree
//============================================================================
#include <algorithm>      // Provides transform for converting course numbers to uppercase
#include <cctype>         // Provides toupper for character conversion
#include <fstream>        // Allows the program to read course data from a file
#include <iostream>       // Provides console input and output
#include <sstream>        // Allows each CSV line to be separated into fields
#include <string>         // Stores course numbers, titles, and user input
#include <unordered_set>  // Tracks course numbers for duplicate and prerequisite checks
#include <utility>
#include <vector>         // Temporarily stores courses and their prerequisites
using namespace std;

// Store the information needed for each course in one structure.
struct Course {
    string number;                 // Unique course number, such as CSCI200
    string title;                  // Full name of the course
    vector<string> prerequisites;  // Zero or more prerequisite course numbers
};

// Remove extra spaces and line-ending characters from the beginning and end.
string trim(const string& text) {
    const size_t first = text.find_first_not_of(" \t\r\n");
    if (first == string::npos) return ""; // Return an empty string for blank input.
    const size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1); // Keep only the useful characters.
}

// Standardize course numbers so searches are not affected by letter case.
string uppercase(string text) {
    transform(text.begin(), text.end(), text.begin(),
        [](unsigned char c) { return static_cast<char>(toupper(c)); });
    return text;
}

// Store courses in a binary search tree ordered by course number.
class BinarySearchTree {
private:
    // Each node contains one course and pointers to smaller/larger course numbers.
    struct Node {
        Course course;
        Node* left;
        Node* right;
        explicit Node(const Course& value) : course(value), left(nullptr), right(nullptr) {}
    };
    Node* root = nullptr; // An empty tree does not have a root node.

    // Recursively delete both subtrees before deleting their parent node.
    static void destroy(Node* node) {
        if (!node) return; // Stop at an empty branch.
        destroy(node->left);
        destroy(node->right);
        delete node; // Release allocated memory to avoid leaks.
    }

    // Insert a course based on alphanumeric comparison of course numbers.
    static Node* insertNode(Node* node, const Course& course) {
        if (!node) return new Node(course); // Create a node at an empty position.
        if (course.number < node->course.number)
            node->left = insertNode(node->left, course); // Smaller numbers go left.
        else if (course.number > node->course.number)
            node->right = insertNode(node->right, course); // Larger numbers go right.
        // Equal numbers are not inserted again, preventing duplicate tree entries.
        return node;
    }

    // In-order traversal prints all courses in alphanumeric order without sorting.
    static void printInOrder(const Node* node) {
        if (!node) return; // End this branch when there is no node.
        printInOrder(node->left); // Print smaller course numbers first.
        cout << node->course.number << ", " << node->course.title << '\n';
        printInOrder(node->right); // Print larger course numbers afterward.
    }

public:
    BinarySearchTree() = default;
    ~BinarySearchTree() { clear(); } // Free tree memory when the object is destroyed.
    // Prevent copying because two trees should not own the same node pointers.
    BinarySearchTree(const BinarySearchTree&) = delete;
    BinarySearchTree& operator=(const BinarySearchTree&) = delete;

    void clear() { destroy(root); root = nullptr; } // Remove all previous courses.
    void insert(const Course& course) { root = insertNode(root, course); }
    bool empty() const { return root == nullptr; } // Check whether data has been loaded.

    // Search only the branch that could contain the requested course number.
    const Course* search(const string& number) const {
        const Node* current = root; // Begin searching at the root.
        while (current) {
            if (number == current->course.number) return &current->course;
            // Move left for smaller numbers or right for larger numbers.
            current = number < current->course.number ? current->left : current->right;
        }
        return nullptr; // The course is not stored in the tree.
    }

    void printCourses() const { printInOrder(root); }
};

// Separate a CSV record into fields; the supplied data does not use quoted commas.
vector<string> splitLine(const string& line) {
    vector<string> fields;
    string field;
    stringstream stream(line);
    while (getline(stream, field, ',')) fields.push_back(trim(field));
    // Preserve a trailing empty field so it can be checked during validation.
    if (!line.empty() && line.back() == ',') fields.push_back("");
    return fields;
}

// Read the file, skip invalid records, validate prerequisites, and build the tree.
bool loadCourses(const string& filename, BinarySearchTree& courses) {
    ifstream file(filename); // Attempt to open the course information file.
    if (!file.is_open()) {
        cout << "Error: Could not open file '" << filename << "'.\n";
        return false; // Keep the previous tree if the file cannot be opened.
    }

    vector<Course> parsed;       // Hold courses until all validation is complete.
    unordered_set<string> numbers; // Quickly check which course numbers exist.
    string line;
    size_t lineNumber = 0;
    while (getline(file, line)) { // Process each record in the file.
        ++lineNumber;
        if (trim(line).empty()) continue; // Ignore blank lines.
        vector<string> fields = splitLine(line);
        // Every valid record must contain a course number and a title.
        if (fields.size() < 2 || fields[0].empty() || fields[1].empty()) {
            cout << "Skipping invalid course on line " << lineNumber << ".\n";
            continue; // Skip this record and keep reading the remaining file.
        }

        Course course;
        course.number = uppercase(fields[0]); // Normalize the identifier.
        course.title = fields[1];
        if (numbers.count(course.number)) { // Reject repeated course numbers.
            cout << "Skipping duplicate course " << course.number << " on line " << lineNumber << ".\n";
            continue;
        }

        bool malformed = false;
        // Fields after the title represent prerequisite course numbers.
        for (size_t i = 2; i < fields.size(); ++i) {
            if (fields[i].empty()) {
                // Empty trailing columns are allowed, but gaps before later values are not.
                for (size_t j = i + 1; j < fields.size(); ++j) {
                    if (!fields[j].empty()) malformed = true;
                }
                break;
            }
            course.prerequisites.push_back(uppercase(fields[i]));
        }
        if (malformed) {
            cout << "Skipping course " << course.number << ": missing prerequisite field.\n";
            continue;
        }
        numbers.insert(course.number); // Record the accepted course identifier.
        parsed.push_back(course); // Save the course for prerequisite validation.
    }

    // Repeat validation because removing one course may invalidate courses that require it.
    bool removed;
    do {
        removed = false;
        vector<Course> valid; // Build a new list containing only retained courses.
        for (const Course& course : parsed) {
            string missing;
            // Every prerequisite must match a course still available in the list.
            for (const string& prerequisite : course.prerequisites) {
                if (!numbers.count(prerequisite)) {
                    missing = prerequisite;
                    break; // One missing prerequisite is enough to reject the course.
                }
            }
            if (!missing.empty()) {
                cout << "Skipping course " << course.number
                    << ": prerequisite " << missing << " is not available.\n";
                numbers.erase(course.number); // Make dependents invalid on a later pass.
                removed = true;
            }
            else {
                valid.push_back(course); // Keep courses with available prerequisites.
            }
        }
        parsed.swap(valid); // Replace the candidate list with the valid list.
    } while (removed); // Continue until no more courses must be removed.

    // Replace the old tree only after the new file has been read and validated.
    courses.clear();
    for (const Course& course : parsed) courses.insert(course);
    cout << parsed.size() << " valid courses loaded successfully.\n";
    return !parsed.empty();
}

// Display one course and the full titles of any prerequisite courses.
void displayCourse(const BinarySearchTree& courses, const string& number) {
    const Course* course = courses.search(uppercase(trim(number)));
    if (!course) {
        cout << "Course not found. Please check the course number.\n";
        return;
    }
    cout << course->number << ", " << course->title << '\n';
    if (course->prerequisites.empty()) {
        cout << "Prerequisites: None\n";
    }
    else {
        cout << "Prerequisites:\n";
        for (const string& prerequisite : course->prerequisites) {
            // Look up each prerequisite so its title can be displayed too.
            const Course* required = courses.search(prerequisite);
            cout << "  " << prerequisite << ", "
                << (required ? required->title : "Unknown course") << '\n';
        }
    }
}

// Display the menu repeatedly until the user exits or input ends.
int main() {
    BinarySearchTree courses; // Holds the current set of validated courses.
    string choice;
    cout << "Welcome to the course planner.\n";
    while (true) {
        cout << "\n1. Load Data Structure.\n"
            << "2. Print Course List.\n"
            << "3. Print Course.\n"
            << "9. Exit\n"
            << "What would you like to do? ";
        if (!getline(cin, choice)) break; // Exit cleanly if input closes.
        choice = trim(choice);
        if (choice == "1") {
            // Ask for a file path, then load and validate its courses.
            string filename;
            cout << "Enter the course data file name: ";
            if (!getline(cin, filename)) break;
            loadCourses(trim(filename), courses);
        }
        else if (choice == "2") {
            // An in-order traversal prints the full course list alphabetically by number.
            if (courses.empty()) cout << "Please load course data first (option 1).\n";
            else {
                cout << "\nHere is a sample schedule:\n";
                courses.printCourses();
            }
        }
        else if (choice == "3") {
            // Search for a specific course and show its prerequisite information.
            if (courses.empty()) cout << "Please load course data first (option 1).\n";
            else {
                string number;
                cout << "What course do you want to know about? ";
                if (!getline(cin, number)) break;
                displayCourse(courses, number);
            }
        }
        else if (choice == "9") {
            cout << "Thank you for using the course planner!\n";
            break; // End the menu loop.
        }
        else {
            cout << "" << choice << " is not a valid option.\n";
        }
    }
    return 0; // Indicate successful program termination.
}
