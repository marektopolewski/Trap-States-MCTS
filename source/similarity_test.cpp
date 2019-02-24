#include <vector>
#include <c++/cmath>

#include <iostream>
#include <fstream>
#include <ctime>

#include "similarity_test.h"
#include "similarity.h"
#include "position.h"
#include "movegen.h"
#include "move.h"

void autoTest();
void manTest();
std::string getTimeStamp();
double simFromKey(int key, std::string fen1, std::string fen2);
std::string trapPersistence(std::string fen1, std::string fen2);
bool isTrap(std::string fen);

/**
 * Test wrapper function. Loop over 'man' and 'auto' methods for testing until 'exit' command encountered.
 */
void similarityTest() {
    std::string cmd="";
    while (true) {
        getline(std::cin, cmd);
        if (cmd == EXIT_CMD) break;
        else if (cmd==AUTO_CMD) autoTest();
        else if (cmd==MANUAL_CMD) manTest();
        else std::cout << "[ERROR] Invalid command." << std::endl;
    }
}

/**
 * Automatic testing scheme.
 */
void autoTest() {
    std::ifstream testFile;
    std::ofstream resultFile;
    std::string resultName = "..\\test\\result_" + getTimeStamp() + ".csv";

    testFile.open("..\\test\\test_set.in");
    resultFile.open(resultName);

    if (!testFile || !resultFile) {
        std::cout << "[ERROR] Unable to open the required file(s)." ;
        std::cout << std::endl;
        return ;
    }

    resultFile << "CONSTANT,DEPTH_BREADTH,INFL_PIECES,"
               << "LEGAL_MOVES,REC_LEGAL_MOVES,EXPANDABLE_STATES,"
               << "REC_EXPANDABLE_STATES,"
               << "trap_presence\n";

    std::string inStr = "", outStr, prevFen, curFen;
    int fenCount;
    double sim;
    while(true) {
        getline(testFile, inStr);
        fenCount = atoi(inStr.c_str());
        if (fenCount < 2) break;            // insufficient number of positions to calculate the similairty

        prevFen = "";
        for (int i=0; i < fenCount; i++) {
            getline(testFile, curFen);
            if (prevFen != "") {
                outStr = "";
                for (int simMethod = CONSTANT; simMethod < REC_EXPANDABLE_STATES; simMethod++) {
                    sim = simFromKey(simMethod, curFen, prevFen);
                    outStr += std::to_string(sim) + ",";
                }
                resultFile << outStr << trapPersistence(curFen, prevFen) << std::endl;
            }
            prevFen = curFen;
        }
        resultFile << std::endl;
    }
    testFile.close();
    resultFile.close();

    std::cout << "[INFO] Results of automatic tests exported to: " << resultName << std::endl;
}

/**
 * Manual testing. Interleave FEN input of two positions and similarity measure selection.
 * Loop until exit' command encountered, then return to the main 'sim' loop.
 */
void manTest() {
    std::string keyStr;
    int key = 0;
    double sim = DEFAULT_SIM;
    while(true) {
        std::cout << "[INFO] Select similairty measure key (or '9' to quit):\n";
        std::cout <<
                  "   Name                  |key\n" <<
                  "   ------------------------- \n" <<
                  "   CONSTANT              | 0 \n" <<
                  "   DEPTH_BREADTH         | 1 \n" <<
                  "   INFL_PIECES           | 2 \n" <<
                  "   LEGAL_MOVES           | 3 \n" <<
                  "   REC_LEGAL_MOVES       | 4 \n" <<
                  "   EXPANDABLE_STATES     | 5 \n" <<
                  "   REC_EXPANDABLE_STATES | 6 \n";
        getline(std::cin, keyStr);
        key = atoi(keyStr.c_str());
        if (key == 9) {
            break;
        }
        if (keyStr.size()>1 || key < CONSTANT || key > REC_EXPANDABLE_STATES) {
            std::cout << "[ERROR] Invalid similairty key" << std::endl;
        }
        else {
            std::string fen1, fen2;
            std::cout << "[INFO] Enter FEN one: ";
            getline(std::cin, fen1);
            std::cout << "[INFO] Enter FEN two: ";
            getline(std::cin, fen2);

            sim = simFromKey(key, fen1, fen2);
            if (sim == INVALID_FEN) break;
        }
    }

    if (sim == INVALID_FEN) std::cout << "[ERROR] Invalid FEN position(s).\n";
    else std::cout << "[INFO] Similairty is: " << sim << "\n\n";
}

/**
 * Due to inability to use variable templates, this wrapper method is used to correctly call the
 * similarityFen<SimMethod>() where SimMethod is the key of the mehtod to use.
 * @param key the unique key of a similairty method
 * @param fen1 the first FEN position stored in a string
 * @param fen2 the second FEN position stored in a string
 * @return Corresponding similairty value between two positions
 */
double simFromKey(int key, std::string fen1, std::string fen2) {
    Position *pos1  = new Position(fen1, false, 0);
    Position *pos2  = new Position(fen2, false, 0);

    double sim = DEFAULT_SIM;
    if (pos1->get_key()==0 || pos2->get_key()==0) {
        return sim;
    }

    switch (key) {
        case 0:
            sim = similarity<CONSTANT>(pos1, pos2);
            break;
        case 1:
            sim = similarity<DEPTH_BREADTH>(pos1, pos2);
            break;
        case 2:
            sim = similarity<LEGAL_MOVES>(pos1, pos2);
            break;
        case 3:
            sim = similarity<REC_LEGAL_MOVES>(pos1, pos2);
            break;
        case 4:
            sim = similarity<REC_LEGAL_MOVES>(pos1, pos2);
            break;
        case 5:
            sim = similarity<EXPANDABLE_STATES>(pos1, pos2);
            break;
        case 6:
            sim = similarity<REC_EXPANDABLE_STATES>(pos1, pos2);
            break;
        default:;
    }

    return sim;
}

/**
 * Generates a current timestamp used to uniquely identify files containig results.
 * @return timestamp in a from of a string
 */
std::string getTimeStamp() {
    time_t now = time(0);
    tm *ltm = localtime(&now);

    std::string ts = "";

    ts += std::to_string(ltm->tm_mday) + "_";
    ts += std::to_string(1 + ltm->tm_mon) + "_";
    ts += std::to_string(1900 + ltm->tm_year) + "__";
    ts += std::to_string(1 + ltm->tm_hour) + "_";
    ts += std::to_string(ltm->tm_min);

    return ts;
}

/**
 * Evaluates trap persistence between two FEN board positions.
 * @param fen1 the first FEN position stored in a string
 * @param fen2 the second FEN position stored in a string
 * @return "both" if trap persists, "fen1" if only in 1st position, "fen2" if only in 2nd position, "none" otherwise.
 */
std::string trapPersistence(std::string fen1, std::string fen2) {
    bool trap1 = isTrap(fen1), trap2 = isTrap(fen2);
    if (trap1 && trap2) return "both";
    if (!trap1 && !trap2) return "none";
    if (trap1) return "fen1";
    if (trap2) return "fen2";
}

/**
 * Checks whether any of the moves of the agent leads to a 2-winning strategy for the opponent.
 * @param fen position stored in a FEN string
 * @return true if the trap is present, false otherwise
 */
bool isTrap(std::string fen) {
    Position *pos  = new Position(fen, false, 0);
    StateInfo st;
    std::vector<MoveStack> ms = getMoves(pos);
    for (auto m : ms) {
        pos->do_move(m.move, st);
        if (pos->is_trap()) return true;
        pos->undo_move(m.move);
    }
    return false;
}