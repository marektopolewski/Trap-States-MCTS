#include <vector>
#include <c++/cmath>

#include <iostream>
#include <fstream>
#include <ctime>

#include "similarity.h"
#include "position.h"
#include "movegen.h"
#include "move.h"

template<SimMethod>
double intersectionRec(Position* curPos, Position* prevPos, std::vector<MoveStack> v1, std::vector<MoveStack> v2);
double intersection(std::vector<MoveStack> v1, std::vector<MoveStack> v2);
std::vector<MoveStack> getMoves(Position* pos);

void autoTest();
void manTest();
std::string getTimeStamp();
double simFromKey(int key, std::string fen1, std::string fen2);
std::string trapPersistence(std::string fen1, std::string fen2);
bool isTrap(std::string fen);

///////////////////////////////////////////////////
/// SIMILARITY MEASURES
///////////////////////////////////////////////////
/**
 * Use a constant value as a static similarity measure. Method employed in the final version of TA-MCTS.
 * @param curPos  (ignored) current board state
 * @param prevPos (ignored) reference board state
 * @return a constant predefined in the header
 */
template<>
double similarity<CONSTANT>(Position* curPos, Position* prevPos) {
    return CONST_SIM;
}

/**
 * Use the weighted sum of trees' depth difference and breadth difference to calculate the similarity.
 * @param curPos current board state
 * @param prevPos reference board state
 * @return similarity between curPos and prevPos
 */
template<>
double similarity<DEPTH_BREADTH>(Position* curPos, Position* prevPos) {
	double curDepth  = curPos->startpos_ply_counter(),
	       prevDepth = prevPos->startpos_ply_counter();

	double curBreadth  = getMoves(curPos).size(),
	       prevBreadth = getMoves(prevPos).size();

    double depthComponent;
    if (curDepth + prevDepth == 0) depthComponent = 1;
    else
        depthComponent =  1 - fabs(curDepth - prevDepth) / (curDepth + prevDepth);

    double breadthComponent;
    if (curBreadth + prevBreadth == 0) breadthComponent = 1;
    else
        breadthComponent = 1 - fabs(curBreadth - prevBreadth) / (curBreadth + prevBreadth);

	return (DEPTH_WEIGHT*depthComponent + BREADTH_WEIGHT*breadthComponent) / (DEPTH_WEIGHT + BREADTH_WEIGHT);
}

/**
 * Use the intersection-to-union ratio of influential elements of the game to calculate the similarity.
 * @param curPos current board state
 * @param prevPos reference board state
 * @return similarity between curPos and prevPos
 */
template<>
double similarity<INFL_PIECES>(Position* curPos, Position* prevPos) {

	if (!prevPos) return DEFAULT_SIM;

	double sum = 0, inter = 0;
	int curPiece, prevPiece;

	for (PieceType pt = PIECE_TYPE_NONE; pt <= KING; pt++) {
		curPiece = curPos->piece_count(curPos->side_to_move(), pt);
		prevPiece = prevPos->piece_count(prevPos->side_to_move(), pt);

		sum += std::max(curPiece, prevPiece);
		inter += std::min(curPiece, prevPiece);
	}
	return inter / sum;
}

/**
 * Use the intersection-to-union ratio of legal actions from each game state to obtain the similarity.
 * @param curPos current board state
 * @param prevPos reference board state
 * @return similarity between curPos and prevPos
 */
template<>
double similarity<LEGAL_MOVES>(Position* curPos, Position* prevPos) {

	if (!prevPos) return DEFAULT_SIM;

	std::vector<MoveStack> curMoves  = getMoves(curPos),
                           prevMoves = getMoves(prevPos);

	double uni = curMoves.size() + prevMoves.size();
	double inter = intersection(curMoves, prevMoves);
	uni = uni - inter;

	return inter / uni;
}

/**
 * Use the intersection-to-union ratio of legal actions from each game state to obtain the similarity.
 * Further expand the tree by recursively applying the formula to action mismatches.
 * @param curPos current board state
 * @param prevPos reference board state
 * @return similarity between curPos and prevPos
 */
template<>
double similarity<REC_LEGAL_MOVES>(Position* curPos, Position* prevPos) {

	if (!prevPos) return DEFAULT_SIM;

    std::vector<MoveStack> curMoves  = getMoves(curPos),
	                       prevMoves = getMoves(prevPos);

    double inter = 0,
           uni   = curMoves.size() + prevMoves.size();

    for (long long i = curMoves.size()-1; i >= 0; i--) {
        if (curMoves.empty()) break;
        for (long long j = prevMoves.size()-1; j >= 0; j--) {
            if (prevMoves.empty()) break;
            if (curMoves[i].move == prevMoves[j].move) {
                curMoves.erase(curMoves.begin()+i);
                prevMoves.erase(prevMoves.begin()+j);
                inter++;
                break;
            }
        }
    }

    double simMismatches = intersectionRec<LEGAL_MOVES>(curPos, prevPos, curMoves, prevMoves);
	uni = uni - inter - simMismatches;
	return (inter + simMismatches) / uni;
}

/**
 * Use the intersection-to-union ratio of expandable positions from each game state to obtain the similarity.
 * @param curPos current board state
 * @param prevPos reference board state
 * @return similarity between curPos and prevPos
 */
template<>
double similarity<EXPANDABLE_STATES>(Position* curPos, Position* prevPos) {

	if (!prevPos) return DEFAULT_SIM;

	std::vector<MoveStack> curMoves = getMoves(curPos),
			               prevMoves = getMoves(prevPos);

	Position *p1, *p2;
	StateInfo st1, st2;
	double inter = 0,
	       uni   = curMoves.size() + prevMoves.size();

	for (unsigned int i = 0; i < curMoves.size(); i++) {
		for (unsigned int j = 0; j < prevMoves.size(); j++) {
			p1 = new Position(*curPos, curPos->thread());
			p2 = new Position(*prevPos, prevPos->thread());
			p1->do_move(curMoves[i].move, st1);
			p2->do_move(prevMoves[j].move, st2);

			if (p1->get_key() == p2->get_key()) {
				prevMoves.erase(prevMoves.begin()+j);
				inter++;
				break;
			}
		}
	}

	uni = uni - inter;
	return inter / uni;
}

/**
 * Use the intersection-to-union ratio of expandable positions from each game state to obtain the similarity.
 * Further expand the tree by recursively applying the formula to position mismatches.
 * @param curPos current board state
 * @param prevPos reference board state
 * @return similarity between curPos and prevPos
 */
template<>
double similarity<REC_EXPANDABLE_STATES>(Position* curPos, Position* prevPos) {

	if (!prevPos) return DEFAULT_SIM;

	std::vector<MoveStack> curMoves = getMoves(curPos),
			prevMoves = getMoves(prevPos);

	double inter = 0,
		   uni   = curMoves.size() + prevMoves.size();

	Position *p1, *p2;
	StateInfo st1, st2;
	for (long long i = curMoves.size()-1; i >= 0; i--) {
        if (curMoves.empty() || prevMoves.empty()) break;
		for (long long j = prevMoves.size()-1; j >= 0; j--) {
			p1 = new Position(*curPos, curPos->thread());
			p2 = new Position(*prevPos, prevPos->thread());
			p1->do_move(curMoves[i].move, st1);
			p2->do_move(prevMoves[j].move, st2);

			if (p1->get_key() == p2->get_key()) {
				curMoves.erase(curMoves.begin()+i);
				prevMoves.erase(prevMoves.begin()+j);
				inter++;
				break;
			}
		}
	}

    double simMismatches = intersectionRec<EXPANDABLE_STATES>(curPos, prevPos, curMoves, prevMoves);
	uni = uni - inter - simMismatches;
	return (inter + simMismatches) / uni;
}


///////////////////////////////////////////////////
/// HELPER METHODS
///////////////////////////////////////////////////
/**
 * Wrapper method that allows to compute a similarity between 2 FEN-encoded board positions.
 * @tparam simMethod function used to compute the similairty
 * @param curFen current board state as FEN string
 * @param prevFen reference board state as FEN string
 * @return similarity between curPos and prevPos
 */
template<SimMethod simMethod>
double similarityFen(std::string curFen, std::string prevFen) {
    Position *curPos  = new Position(curFen, false, 0);
    Position *prevPos = new Position(prevFen, false, 0);

    if (curPos->get_key()==0 || prevPos->get_key()==0) {
        return INVALID_FEN;
    }

    return similarity<simMethod>(curPos, prevPos);
}

/**
 * Generate a set of legal moves from a given position and store it in a vector.
 * @param pos board state to consider
 * @return a vector of extendable moves
 */
std::vector<MoveStack> getMoves(Position* pos) {
    MoveStack moveList[MAX_MOVES];
    std::vector<MoveStack> vec;

    MoveStack *last = generate<MV_LEGAL>(*pos, moveList);
    for (MoveStack *i = moveList; i != last; i++)
        vec.push_back(*i);

    return vec;
}

/**
 * Perform an intersection of two sets of moves and return its cardinality.
 * @param v1 first set of moves stored as a vector
 * @param v2 second set of moves stored as a vector
 * @return Size of the intersection
 */
double intersection(std::vector<MoveStack> v1, std::vector<MoveStack> v2) {
    double inter = 0;
    for (unsigned int i = 0; i < v1.size(); i++) {
        for (unsigned int j = 0; j < v2.size(); j++) {
            if (v1[i].move == v2[j].move) {
                v2.erase(v2.begin()+j);		// remove matches from consideration for speed-up
                inter++; 						// string s = move_to_uci(v2[j].move, false);
                break;
            }
        }
    }
    return inter;
}

/**
 * Perform an intersection of two sets of moves. Mismatching moves are compared using a specified similarity measure
 * and upon exceeding a minimal similarity threshold are considered as equivalent. Return the cardinality of the union
 * of both (1) the intersection and (2) the sufficiently similar mismatches.
 * @tparam simMethod similairty method used to compare the mismatches
 * @param curPos current board state
 * @param prevPos reference board state
 * @param v1 set of mismatched moves from curPos
 * @param v2 set of mismatched moves from prevPos
 * @return sum of the intersection's size and the number similar moves
 */
template<SimMethod simMethod>
double intersectionRec(Position* curPos, Position* prevPos,
                       std::vector<MoveStack> v1, std::vector<MoveStack> v2) {

    Position *p1, *p2;
    StateInfo st1, st2{};
    double inter = 0;

    for (unsigned int i = 0; i < v1.size(); i++) {
        for (unsigned int j = 0; j < v2.size(); j++) {
            p1 = new Position(*curPos, curPos->thread());
            p2 = new Position(*prevPos, prevPos->thread());
            p1->do_move(v1[i].move, st1);
            p2->do_move(v2[j].move, st2);

            if (p1->get_key() == p2->get_key()) {
                inter += REC_INCREMENT;
                break;
            }

            double sim = similarity<simMethod>(p1, p2);
            if (sim > ACC_THRESHOLD) {
                inter += REC_INCREMENT; // alternatively: sim * REC_INCREMENT
                break;
            }
        }
    }
    return inter;
}

///////////////////////////////////////////////////
/// TESTING
///////////////////////////////////////////////////

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
    double sim = DEFAULT_SIM;
    switch (key) {
        case 0:
            sim = similarityFen<CONSTANT>(fen1, fen2);
            break;
        case 1:
            sim = similarityFen<DEPTH_BREADTH>(fen1, fen2);
            break;
        case 2:
            sim = similarityFen<LEGAL_MOVES>(fen1, fen2);
            break;
        case 3:
            sim = similarityFen<REC_LEGAL_MOVES>(fen1, fen2);
            break;
        case 4:
            sim = similarityFen<REC_LEGAL_MOVES>(fen1, fen2);
            break;
        case 5:
            sim = similarityFen<EXPANDABLE_STATES>(fen1, fen2);
            break;
        case 6:
            sim = similarityFen<REC_EXPANDABLE_STATES>(fen1, fen2);
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