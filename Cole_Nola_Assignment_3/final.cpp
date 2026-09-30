/*
 * Program:        EECS 348 Assignment 3 - CEO Email Prioritizer
 * Description:    C++ program that stores a CEO's emails in a MaxHeap priority
 *                 queue. Priority order: Boss, Subordinate,
 *                 Peer, ImportantPerson, OtherPerson. Within a category, the
 *                 newest date is read first. Commands: EMAIL, NEXT, READ, COUNT.
 * Inputs:         Test file (path given as argv[1]) or stdin if no argument.
 * Output:         Terminal output for the COUNT and NEXT commands.
 * Collaborators:  None
 * Other sources:  Gemini C++ initial code (basis for this program), ChatGPT
 *                 initial code (compared in the GenAI analysis) and Claude
 *                 (Anthropic) for the improvements and comments.
 * Author:         Cole Nola
 * Creation date:  9/30/2026
 * Revision date:  9/30/2026
 * Revisions:      Precomputed rank and date key in the Email constructor,
 *                 changed the tiebreak to latest-arrival-first, replaced
 *                 stringstream/stoi parsing with find/rfind and a manual digit
 *                 parse, added CRLF handling, trimming, and file input.
 */
#include <iostream>   // std::cin, std::cout, std::cerr
#include <fstream>    // std::ifstream for reading the test file
#include <string>     // std::string
#include <vector>     // std::vector used as the heap's underlying list storage
#include <utility>    // std::move, std::swap
 
// ---------------------------------------------------------------
// Email class. Structure based on Gemini's Email struct; the
// precomputed keys, constructor, and digit parser were added by Claude.
// ---------------------------------------------------------------
class Email {
public:
    // Builds an email and computes its priority keys once (Claude).
    Email(const std::string& sender, const std::string& subject,
          const std::string& date, unsigned long seq)
        : sender_(sender), subject_(subject), date_(date), seq_(seq) {
        rank_ = computeRank(sender);                  // numeric sender priority
        if (date.size() >= 10) {                      // expect MM-DD-YYYY
            dateKey_ = toInt(date, 6, 4) * 10000      // year
                     + toInt(date, 0, 2) * 100        // month
                     + toInt(date, 3, 2);             // day
        } else {
            dateKey_ = 0;                             // malformed date sorts oldest
        }
    }
 
    // Returns true if this email must be read before 'other'.
    // Order logic follows Gemini's operator>; tiebreak changed by Claude.
    bool higherPriorityThan(const Email& other) const {
        if (rank_ != other.rank_) return rank_ > other.rank_;              // category first
        if (dateKey_ != other.dateKey_) return dateKey_ > other.dateKey_;  // newer date first
        return seq_ > other.seq_;   // same category and date: later arrival first
    }
 
    // Accessors used when printing (Claude).
    const std::string& sender() const { return sender_; }
    const std::string& subject() const { return subject_; }
    const std::string& date() const { return date_; }
 
private:
    // Maps category name to priority; Boss highest, unknown = OtherPerson
    // (mapping from Gemini's getCategoryPriority).
    static int computeRank(const std::string& s) {
        if (s == "Boss") return 5;
        if (s == "Subordinate") return 4;
        if (s == "Peer") return 3;
        if (s == "ImportantPerson") return 2;
        return 1;
    }
 
    // Parses 'len' digits of s starting at pos; returns 0 if any character
    // is not a digit. Replaces stoi so bad input cannot throw (Claude).
    static int toInt(const std::string& s, size_t pos, size_t len) {
        int v = 0;                                    // running value
        for (size_t i = pos; i < pos + len; ++i) {    // walk each digit
            if (s[i] < '0' || s[i] > '9') return 0;   // reject non-digits
            v = v * 10 + (s[i] - '0');                // append the digit
        }
        return v;                                     // parsed number
    }
 
    std::string sender_, subject_, date_;  // original text kept for output
    int rank_;                             // sender priority (5 = Boss ... 1 = OtherPerson)
    int dateKey_;                          // YYYYMMDD integer, larger = newer
    unsigned long seq_;                    // arrival order, used as tiebreaker
};
 
// ---------------------------------------------------------------
// MaxHeap class. Written from scratch; no std heap functions are used.
// Sift-up/sift-down logic adapted from Gemini's heapifyUp/heapifyDown.
// ---------------------------------------------------------------
class MaxHeap {
public:
    // Adds an email and restores the heap property. O(log n).
    void insert(Email e) {
        data_.push_back(std::move(e));            // place at the end of the list
        siftUp(data_.size() - 1);                 // move up to its position
    }
 
    // Returns the highest-priority email without removing it, or nullptr. O(1).
    const Email* peek() const {
        return data_.empty() ? nullptr : &data_.front();
    }
 
    // Removes the highest-priority email; no-op if empty. O(log n).
    void removeMax() {
        if (data_.empty()) return;                // READ on empty inbox does nothing
        data_.front() = std::move(data_.back());  // last element replaces the root
        data_.pop_back();                         // shrink the list by one
        if (!data_.empty()) siftDown(0);          // restore the heap property
    }
 
    // Number of unread emails. O(1).
    size_t size() const { return data_.size(); }
 
private:
    // Moves the element at index i up while it outranks its parent.
    void siftUp(size_t i) {
        while (i > 0) {
            size_t parent = (i - 1) / 2;          // parent index
            if (data_[i].higherPriorityThan(data_[parent])) {
                std::swap(data_[i], data_[parent]);   // child outranks parent: swap
                i = parent;                           // continue from the parent slot
            } else {
                break;                                // heap property holds
            }
        }
    }
 
    // Moves the element at index i down while a child outranks it.
    void siftDown(size_t i) {
        size_t n = data_.size();                  // current heap size
        while (true) {
            size_t l = 2 * i + 1;                 // left child index
            size_t r = 2 * i + 2;                 // right child index
            size_t best = i;                      // index of the highest priority so far
            if (l < n && data_[l].higherPriorityThan(data_[best])) best = l;
            if (r < n && data_[r].higherPriorityThan(data_[best])) best = r;
            if (best == i) break;                 // already in the correct position
            std::swap(data_[i], data_[best]);     // swap with the stronger child
            i = best;                             // continue from the child slot
        }
    }
 
    std::vector<Email> data_;   // list-based (array) heap storage
};
 
// Removes leading and trailing spaces from a string (Claude).
static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(' ');          // first non-space
    if (a == std::string::npos) return "";        // all spaces or empty
    size_t b = s.find_last_not_of(' ');           // last non-space
    return s.substr(a, b - a + 1);                // trimmed copy
}
 
// Main processing loop. Command handling follows Gemini's main; file input,
// CRLF handling, and find/rfind parsing were added by Claude.
int main(int argc, char* argv[]) {
    std::ifstream file;                           // used only if a file is given
    std::istream* in = &std::cin;                 // default: read from stdin
    if (argc > 1) {                               // a file path was supplied
        file.open(argv[1]);
        if (!file) {                              // could not open the file
            std::cerr << "Cannot open " << argv[1] << "\n";
            return 1;
        }
        in = &file;                               // read from the file instead
    }
 
    MaxHeap heap;                                 // the CEO's inbox
    unsigned long counter = 0;                    // arrival counter for tiebreaks
    std::string line;                             // current input line
    while (std::getline(*in, line)) {             // one command per line
        if (!line.empty() && line.back() == '\r') line.pop_back();  // CRLF safety
 
        if (line.compare(0, 6, "EMAIL ") == 0) {          // EMAIL command
            std::string body = line.substr(6);            // text after "EMAIL "
            size_t c1 = body.find(',');                   // first comma
            size_t c2 = body.rfind(',');                  // last comma
            if (c1 != std::string::npos && c2 != c1) {    // need two commas
                heap.insert(Email(trim(body.substr(0, c1)),             // sender category
                                  trim(body.substr(c1 + 1, c2 - c1 - 1)), // subject line
                                  trim(body.substr(c2 + 1)),            // date
                                  ++counter));                          // arrival number
            }
        } else if (line == "NEXT") {                      // NEXT command
            const Email* e = heap.peek();                 // highest-priority email
            if (e) {                                      // empty inbox prints nothing
                std::cout << "Next email:\n"
                          << "Sender: " << e->sender() << "\n"
                          << "Subject: " << e->subject() << "\n"
                          << "Date: " << e->date() << "\n";
            }
        } else if (line == "READ") {                      // READ command
            heap.removeMax();                             // delete top email, no output
        } else if (line == "COUNT") {                     // COUNT command
            std::cout << "There are " << heap.size() << " emails to read.\n";
        }
    }
    return 0;
}
