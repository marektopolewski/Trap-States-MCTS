/*
 * montecarlo.h
 *
 *  Created on: 14.12.2011
 *      Author: oleg
 */

#ifndef MONTECARLO_H_
#define MONTECARLO_H_

#include "move.h"
#include "types.h"
#include <vector>

const int MAX_PLY = 255;
const int BLACK_MATES_IN_ONE = -INT_MAX;
const int WHITE_MATES_IN_ONE = INT_MAX;

class MonteCarloTreeNode {

public:
	MonteCarloTreeNode(Move move, MonteCarloTreeNode * parentNode, Value score);
	~MonteCarloTreeNode();
	MonteCarloTreeNode * UCT_select(Position * rootPosition);
	MonteCarloTreeNode * UCT_expand(Position * rootPosition);
	double simulate(double sim, Position * rootPosition);
	void update(double value, Position * root);
	void normalUpdate(double value);
	std::vector<Move> movesFromRoot();
	void printMultiPv(int depth, int iterations, int searchTime, bool whiteToMove, int UCIMultiPV);
	std::string pv_info_to_uci(int depth, unsigned long int iterations, int searchTime, bool whiteToMove, int multipv);
	Move lastMove;
	int visits;
	double simcounter; // NEW
	MonteCarloTreeNode * bestChild();


private:
	void updateKnownWin(double value, bool whiteToMove);
	MonteCarloTreeNode * addChild(Move move, Value score);
	Value heuristicScore;
	size_t maxMoves;
	double totalValue;
	MonteCarloTreeNode * parent;
	std::vector<MonteCarloTreeNode *> children;
};

#endif /* MONTECARLO_H_ */
