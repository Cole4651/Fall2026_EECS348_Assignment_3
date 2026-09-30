#include <iostream>
#include <string>
#include <vector>
#include <sstream>

// Representation of an Email
struct Email {
    std::string sender;
    std::string subject;
    std::string date; // MM-DD-YYYY
    int insertionIndex; // Secondary key for stable tracking

    // Helper to map sender categories to numeric priority (higher = read first)
    int getCategoryPriority() const {
        if (sender == "Boss") return 5;
        if (sender == "Subordinate") return 4;
        if (sender == "Peer") return 3;
        if (sender == "ImportantPerson") return 2;
        if (sender == "OtherPerson") return 1;
        return 0;
    }

    // Helper to convert MM-DD-YYYY to YYYYMMDD for strict chronological comparison
    int getDateComparable() const {
        if (date.length() < 10) return 0;
        int month = std::stoi(date.substr(0, 2));
        int day   = std::stoi(date.substr(3, 2));
        int year  = std::stoi(date.substr(6, 4));
        return year * 10000 + month * 100 + day;
    }

    // Returns true if *this* email has STRICTLY HIGHER priority than *other*
    bool operator>(const Email& other) const {
        int p1 = getCategoryPriority();
        int p2 = other.getCategoryPriority();

        if (p1 != p2) {
            return p1 > p2;
        }

        // Same category -> Newest email first (larger YYYYMMDD value)
        int d1 = getDateComparable();
        int d2 = other.getDateComparable();
        if (d1 != d2) {
            return d1 > d2;
        }

        // Tiebreaker for identical category and date -> preserve FIFO/LIFO consistency
        return insertionIndex < other.insertionIndex;
    }
};

// Custom MaxHeap implementation built from scratch
class MaxHeap {
private:
    std::vector<Email> heap;

    void heapifyUp(int index) {
        while (index > 0) {
            int parent = (index - 1) / 2;
            // If current node has higher priority than parent, swap
            if (heap[index] > heap[parent]) {
                std::swap(heap[index], heap[parent]);
                index = parent;
            } else {
                break;
            }
        }
    }

    void heapifyDown(int index) {
        int size = heap.size();
        while (true) {
            int left = 2 * index + 1;
            int right = 2 * index + 2;
            int largest = index;

            if (left < size && heap[left] > heap[largest]) {
                largest = left;
            }
            if (right < size && heap[right] > heap[largest]) {
                largest = right;
            }

            if (largest != index) {
                std::swap(heap[index], heap[largest]);
                index = largest;
            } else {
                break;
            }
        }
    }

public:
    void push(const Email& email) {
        heap.push_back(email);
        heapifyUp(heap.size() - 1);
    }

    void pop() {
        if (heap.empty()) return;
        heap[0] = heap.back();
        heap.pop_back();
        if (!heap.empty()) {
            heapifyDown(0);
        }
    }

    Email top() const {
        return heap.front();
    }

    bool empty() const {
        return heap.empty();
    }

    size_t size() const {
        return heap.size();
    }
};

// Main processing logic
int main() {
    MaxHeap emailQueue;
    std::string line;
    int globalInsertionCounter = 0;

    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        if (line.rfind("EMAIL ", 0) == 0) {
            std::string payload = line.substr(6);
            std::stringstream ss(payload);
            
            std::string sender, subject, date;
            std::getline(ss, sender, ',');
            std::getline(ss, subject, ',');
            std::getline(ss, date, ',');

            // Trim leading spaces if any
            if (!sender.empty() && sender[0] == ' ') sender = sender.substr(1);
            if (!subject.empty() && subject[0] == ' ') subject = subject.substr(1);
            if (!date.empty() && date[0] == ' ') date = date.substr(1);

            Email email{sender, subject, date, globalInsertionCounter++};
            emailQueue.push(email);

        } else if (line == "NEXT") {
            if (!emailQueue.empty()) {
                Email topEmail = emailQueue.top();
                std::cout << "Next email:\n";
                std::cout << "Sender: " << topEmail.sender << "\n";
                std::cout << "Subject: " << topEmail.subject << "\n";
                std::cout << "Date: " << topEmail.date << "\n";
            }
        } else if (line == "READ") {
            if (!emailQueue.empty()) {
                emailQueue.pop();
            }
        } else if (line == "COUNT") {
            std::cout << "There are " << emailQueue.size() << " emails to read.\n";
        }
    }

    return 0;
}

