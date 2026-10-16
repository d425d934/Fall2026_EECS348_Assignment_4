/*
 * Program name:   EECS 348 Assignment 4 - Sudoku Solver
 * Description:    Solves 5 sudoku puzzles using recursion and backtracking,
 *                 printing every solution it finds.
 * Inputs:         puzzle1.txt - puzzle5.txt (digits 1-9, _ for a blank)
 * Output:         File name, the original puzzle, and all solutions or
 *                 "No solution found"
 * Collaborators:  None
 * Other sources:  ChatGPT (GPT-6, chatgpt.com) - base program
 *                 Gemini 3.5 Flash-Lite (gemini.google.com) - board/solver
 *                 class split, file name in solution header
 * Author:         Daveion Devres
 * Creation date:  10/15/2026
 */

#include <fstream>   // reading files
#include <iostream>  // cout
#include <string>    // strings

// Board class: grid + which digits are used in each row/col/block
// Source: Gemini (two-class idea), ChatGPT (grid, loading, printing),
// Me (bitmasks, candidate list, cell picking)
class SudokuBoard {
public:
    static const int SIZE = 9;                // 9x9 grid
    static const int BOX = 3;                 // 3x3 blocks
    static const unsigned short ALL = 0x1FF;  // all 9 digits as bits

private:
    unsigned char cells[SIZE][SIZE];  // 0 = blank
    unsigned short rowUsed[SIZE];     // digits used per row
    unsigned short colUsed[SIZE];     // digits used per column
    unsigned short boxUsed[SIZE];     // digits used per block
    bool givensValid;                 // false if the file repeats a digit

    // block number 0-8 for a cell - Source: Me
    static int boxIndex(int row, int col) {
        return (row / BOX) * BOX + col / BOX;  // block row * 3 + block col
    }

    // counts set bits = number of candidates - Source: Me
    static int countBits(unsigned short mask) {
        int count = 0;          // bits counted
        while (mask != 0) {     // until no bits left
            mask &= mask - 1;   // drop lowest bit
            ++count;            // count it
        }
        return count;
    }

public:
    // empty board - Source: ChatGPT
    SudokuBoard() {
        clear();  // start blank
    }

    // reset everything - Source: Me
    void clear() {
        for (int r = 0; r < SIZE; ++r) {      // each row
            for (int c = 0; c < SIZE; ++c) {  // each col
                cells[r][c] = 0;              // blank
            }
            rowUsed[r] = 0;                   // nothing used
            colUsed[r] = 0;
            boxUsed[r] = 0;
        }
        givensValid = true;                   // no conflicts yet
    }

    // put a digit in, returns false if it breaks a rule - Source: Me
    bool place(int row, int col, int digit) {
        unsigned short bit = 1 << (digit - 1);  // bit for this digit
        int box = boxIndex(row, col);           // its block
        bool ok = !(rowUsed[row] & bit) &&      // not in row
                  !(colUsed[col] & bit) &&      // not in col
                  !(boxUsed[box] & bit);        // not in block
        cells[row][col] = static_cast<unsigned char>(digit);  // store it
        rowUsed[row] |= bit;                    // mark used
        colUsed[col] |= bit;
        boxUsed[box] |= bit;
        return ok;
    }

    // take a digit back out (backtracking) - Source: Me
    void remove(int row, int col) {
        unsigned short bit = 1 << (cells[row][col] - 1);  // bit to clear
        rowUsed[row] &= ~bit;                             // free in row
        colUsed[col] &= ~bit;                             // free in col
        boxUsed[boxIndex(row, col)] &= ~bit;              // free in block
        cells[row][col] = 0;                              // blank again
    }

    // load a puzzle, error gets set if something is wrong
    // Source: ChatGPT (file checks), Me (reads straight into the grid,
    // more-than-81-cells check)
    bool loadFromFile(const std::string& filename, std::string& error) {
        clear();                                   // fresh board
        std::ifstream input(filename.c_str());     // open file
        if (!input.is_open()) {                    // can't open
            error = "Could not open " + filename;
            return false;
        }
        char ch;                                   // current entry
        for (int i = 0; i < SIZE * SIZE; ++i) {    // read 81 entries
            if (!(input >> ch)) {                  // file too short
                error = filename + " has fewer than 81 cells";
                return false;
            }
            int row = i / SIZE;                    // row of entry
            int col = i % SIZE;                    // col of entry
            if (ch == '_') {                       // blank, skip
                continue;
            }
            if (ch < '1' || ch > '9') {            // bad character
                error = std::string("Invalid character '") + ch + "' in " + filename;
                return false;
            }
            if (!place(row, col, ch - '0')) {      // store digit
                givensValid = false;               // repeated digit in file
            }
        }
        if (input >> ch) {                         // extra stuff at the end
            error = filename + " has more than 81 cells";
            return false;
        }
        return true;
    }

    // true if the starting puzzle follows the rules - Source: ChatGPT (idea), Me
    bool hasValidGivens() const {
        return givensValid;
    }

    // digits that can go in this cell, as bits - Source: Me
    unsigned short candidates(int row, int col) const {
        unsigned short used = rowUsed[row] | colUsed[col] |
                              boxUsed[boxIndex(row, col)];  // digits taken
        return ALL & ~used;                                 // the rest are allowed
    }

    // find the blank cell with the fewest candidates
    // returns false if there are no blanks (solved) - Source: Me
    bool findBestEmptyCell(int& bestRow, int& bestCol, unsigned short& bestMask) const {
        int bestCount = SIZE + 1;                    // higher than possible
        for (int r = 0; r < SIZE; ++r) {             // each row
            for (int c = 0; c < SIZE; ++c) {         // each col
                if (cells[r][c] != 0) {              // skip filled cells
                    continue;
                }
                unsigned short mask = candidates(r, c);  // what fits here
                int count = countBits(mask);             // how many fit
                if (count < bestCount) {             // new best
                    bestCount = count;
                    bestRow = r;
                    bestCol = c;
                    bestMask = mask;
                    if (count <= 1) {                // can't beat 0 or 1
                        return true;
                    }
                }
            }
        }
        return bestCount <= SIZE;                    // found a blank?
    }

    // print grid with block lines - Source: ChatGPT
    void print() const {
        for (int r = 0; r < SIZE; ++r) {              // each row
            if (r % BOX == 0 && r != 0) {             // line between blocks
                std::cout << "------+-------+------\n";
            }
            for (int c = 0; c < SIZE; ++c) {          // each col
                if (c % BOX == 0 && c != 0) {         // divider between blocks
                    std::cout << "| ";
                }
                if (cells[r][c] == 0) {               // blank
                    std::cout << "_ ";
                } else {                              // digit
                    std::cout << static_cast<int>(cells[r][c]) << " ";
                }
            }
            std::cout << '\n';                        // next row
        }
    }
};

// Solver class: loads one file and finds all solutions recursively
// Source: ChatGPT (flow, recursion), Gemini (file name in solution header),
// Me (only trying candidates)
class SudokuSolver {
private:
    SudokuBoard board;     // the puzzle
    std::string filename;  // puzzle file name
    int solutionCount;     // solutions found

    // recursive search, fills one cell per call and backtracks
    // Source: ChatGPT, changed by Me to only try candidates
    void search() {
        int row = 0;                                  // cell to fill
        int col = 0;
        unsigned short mask = 0;                      // its candidates
        if (!board.findBestEmptyCell(row, col, mask)) {  // no blanks = solved
            ++solutionCount;
            std::cout << "\nSolution " << solutionCount << " for " << filename << ":\n";
            board.print();
            return;                                   // keep looking for more
        }
        // mask of 0 means dead end, loop does nothing and we back up
        for (int digit = 1; digit <= SudokuBoard::SIZE; ++digit) {  // try 1-9
            if (mask & (1 << (digit - 1))) {          // only candidates
                board.place(row, col, digit);         // try it
                search();                             // recurse
                board.remove(row, col);               // undo
            }
        }
    }

public:
    // store the file name - Source: Gemini
    explicit SudokuSolver(const std::string& name)
        : filename(name), solutionCount(0) {}

    // load, print, solve, print results - Source: ChatGPT, Me (error output)
    void run() {
        std::cout << "=================================\n";
        std::cout << "Puzzle file: " << filename << '\n';    // file name
        std::cout << "=================================\n";

        std::string error;                                    // load error message
        if (!board.loadFromFile(filename, error)) {           // bad file
            std::cout << "\nError: " << error << "\n";
            std::cout << "\nNo solution found\n\n";
            return;
        }

        std::cout << "\nOriginal puzzle:\n";
        board.print();                                        // show puzzle

        solutionCount = 0;                                    // reset count
        if (board.hasValidGivens()) {                         // only solve valid puzzles
            search();
        }

        if (solutionCount == 0) {                             // none found
            std::cout << "\nNo solution found\n\n";
        } else {                                              // print total
            std::cout << "\nTotal solutions found: " << solutionCount << "\n\n";
        }
    }
};

// solve puzzle1 - puzzle5 - Source: ChatGPT
int main() {
    for (int i = 1; i <= 5; ++i) {                                // 5 puzzles
        std::string name = "puzzle" + std::to_string(i) + ".txt"; // file name
        SudokuSolver solver(name);                                // new solver each time
        solver.run();                                             // solve it
    }
    return 0;
}
