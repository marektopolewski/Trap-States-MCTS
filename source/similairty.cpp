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

    double sim = similarity<simMethod>(curPos, prevPos);
    return sim;
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