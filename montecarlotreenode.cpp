#include "montecarlotreenode.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>

#include "movegen.h"
#include "position.h"
#include "rkiss.h"
#include "evaluate.h"
#include "uctsearch.h"

static RKISS rk;

// Class Constructor, initializes node variables
MonteCarloTreeNode::MonteCarloTreeNode(Move move, MonteCarloTreeNode * parentNode, Value score) {
	maxMoves = MAX_MOVES;
	visits = 0;
	totalValue = 0;
	parent = parentNode;
	lastMove = move;
	heuristicScore = score;
	simcounter = 1; // NEW
}

// Destructor, deletes this Node and all child Nodes
MonteCarloTreeNode::~MonteCarloTreeNode() {
	std::vector<MonteCarloTreeNode*>::iterator child;
	for (child = children.begin(); child < children.end(); child++){
		delete *child;
	}
	children.clear();
}

// Creates a new TreeNode for the given move and adds it as a child of this node
MonteCarloTreeNode * MonteCarloTreeNode::addChild(Move move, Value score) {
	MonteCarloTreeNode * newChild = new MonteCarloTreeNode(move, this, score);
	children.push_back(newChild);

	MonteCarloTreeNode * child = children.back();
	return child;
}

// Returns the move sequence leading from the root position to the position of the current node
std::vector<Move> MonteCarloTreeNode::movesFromRoot() {
	std::vector<Move> moves;
	if (!parent) {
		return moves;
	}
	moves = parent->movesFromRoot();
	moves.push_back(lastMove);
	return moves;
}

// Returns the position of the current node
Position * getTreeNodePosition(MonteCarloTreeNode * node, Position * rootPosition) {
	std::vector<Move> movesFromRoot = node->movesFromRoot();
	Position *pos = new Position((*rootPosition), rootPosition->thread());
	std::vector<Move>::iterator move;
	for (move=movesFromRoot.begin(); move < movesFromRoot.end(); move++){
		pos->do_setup_move(*move);
	}
	return pos;
}

bool whiteWins(double value) {
	return (value >= WHITE_MATES_IN_ONE - MAX_PLY);
}

bool blackWins(double value) {
	return (value <= BLACK_MATES_IN_ONE + MAX_PLY);
}

MonteCarloTreeNode * MonteCarloTreeNode::UCT_select(Position * rootPosition) {
	MonteCarloTreeNode * cur = this;
	MonteCarloTreeNode * chosen = this;
	double bestVal;
	Position * Pos = getTreeNodePosition(this, rootPosition);
	bool whiteToMove;
	if (Pos->side_to_move() == WHITE)
		whiteToMove = true;
	else whiteToMove = false;

	while (cur->children.size() > 0) {
		bestVal = -INT_MAX;
		std::vector<MonteCarloTreeNode*>::iterator child;

		// select this node, if not every legal move was expanded yet
		if (cur->children.size() < cur->maxMoves) {
			delete Pos;
			return cur;
		}

		// every legal move was expanded, step into the next node according to UCT
		double uctVal;

		for (child = cur->children.begin(); child < cur->children.end(); child++){
			double winningrate = ((*child)->totalValue / (*child)->visits);
			if (Pos->side_to_move() == BLACK)
				winningrate = 1 - winningrate;
			uctVal = winningrate + sqrt(2* log( (double) cur->visits)/ (*child)->visits) + 0.001 * (((int)(*child)->heuristicScore) / (*child)->visits);
			if (uctVal > bestVal) {
				chosen = &(**child);
				bestVal = uctVal;
			}
		}

		Pos->do_setup_move(chosen->lastMove);
		whiteToMove = !whiteToMove;
		cur = chosen;
	}
	delete Pos;
	return cur;
}

MonteCarloTreeNode * MonteCarloTreeNode::UCT_expand(Position * rootPosition) {
	MoveStack mlist[MAX_MOVES];
	MoveStack * lastLegal;

	// Generate all legal moves
	Position * pos = getTreeNodePosition(this, rootPosition);

	if (pos->is_really_draw() || pos->is_mate()) {
		delete pos;
		return this;
	}

	MoveStack* last = generate<MV_LEGAL>(*pos, mlist);
	// maxMoves is set when the node is expanded the first time.
	// We don't set it on initialization of the node to avoid unnecessary move generations.
	// Saving the unexpanded Moves in the TreeNode is to memory consuming.
	if (maxMoves == (size_t) MAX_MOVES)
		maxMoves = last - mlist;

	if (maxMoves <= (children.size())) {
		delete pos;
		return this;
	}

	lastLegal = mlist+children.size();
	Value margin;
	Value score = VALUE_ZERO; // (Value) pos->see(lastLegal->move);
	pos->do_setup_move(lastLegal->move);
	score -= evaluate(*pos, margin);
	delete pos;
	return addChild(lastLegal->move, score);
}

bool stmHasDecisiveMove(Position * pos, MoveStack * mlist, MoveStack * last) {
	StateInfo st;
	CheckInfo ci(*pos);

	// On the huge benefit of decisive moves
	for (MoveStack* cur = mlist; cur != last; cur++) {
		if (pos->move_gives_check(cur->move)) {
			pos->do_move(cur->move, st, ci, true);
			if (pos->is_mate()) {
				pos->undo_move(cur->move);
				return true;
			}
			pos->undo_move(cur->move);
		}
	}
	return false;
}

int pickMoveBySee(MoveStack * mlist, MoveStack * last, Position * pos) {
	int i=0;
	int index=-1;
	int maxscore=0;
	int minscore=0;
	for (MoveStack* cur = mlist; cur != last; cur++) {
		cur->score = pos->see(cur->move);
		if (cur->score > maxscore) {
			maxscore = cur->score;
			index=i;
		}
		if (cur->score < minscore) {
			minscore = cur->score;
		}
		i++;
	}

	if (index==-1)
		index = rk.rand<unsigned int>() % (last-mlist);
	return index;
}

double MonteCarloTreeNode::simulate(double sim, Position * rootPosition) {
	//simcounter = simcounter + 0.001;
	simcounter = exp(simcounter - 1 + 0.001);

	MoveStack mlist[MAX_MOVES];
	MoveStack* last;

	int numMoves, index;
	Position * pos = getTreeNodePosition(this, rootPosition);

	if (pos->is_draw()) {
		delete pos;
		return simcounter*0.5;
	}

	if (pos->is_mate()) {
		if (pos->side_to_move() == WHITE) {
			delete pos;
			return BLACK_MATES_IN_ONE;
		}
		else {
			delete pos;
			return WHITE_MATES_IN_ONE;
		}
	}

	last = generate<MV_LEGAL>(*pos, mlist);
	numMoves = last - mlist;

	// PRNG sequence should be non deterministic
	for (int i = abs(get_system_time() % 50); i > 0; i--)
		rk.rand<unsigned>();

	while (!pos->is_draw() && !pos->is_mate()) {

		// Generate all legal moves
		last = generate<MV_LEGAL>(*pos, mlist);
		numMoves = last - mlist;

		// stalemate positions are not recognized by pos->is_draw()
		if (numMoves == 0) {
			delete pos;
			return 0.5; // simcounter*0.5;
		}

		// Check for decisive moves
		if (stmHasDecisiveMove(pos, mlist, last)) {
			if (pos->side_to_move() == WHITE) {
				delete pos;
				return 1; // simcounter*1;
			}
			else {
				delete pos;
				return 0;
			}
		}

		StateInfo st;

		if (rk.rand<unsigned int>() % 10 < 6) {
			index = pickMoveBySee(mlist, last, pos);
			pos->do_move(mlist[index].move, st);
		}
		else {
			index = rk.rand<unsigned int>() % numMoves;
			pos->do_move(mlist[index].move, st);
		}

		// Check for trap (if this was in the old trap list, backpropagate it
		// with probability p = similarity or 0.5)
		if (pos->is_trap()) {
			//if (stmHasDecisiveMove(pos, mlist, last)) {
			if (trapcheck(mlist[index].move)) {
				if (rk.rand<unsigned int>() % 100 < 100 * sim) {
					//if (rk.rand<unsigned int>() % 10 < 6) {
					if (pos->side_to_move() == WHITE) {
						delete pos;
						return 1; // simcounter*1;
					} else {
						delete pos;
						return 0;
					}
				}
			}
			// Else pick another move randomly (don't do this as it avoids failure)
			//pos->undo_move(mlist[index].move);
			//index = rk.rand<unsigned int>() % numMoves;
			//pos->do_move(mlist[index].move, st);
		}

		// play chosen move
		pos->undo_move(mlist[index].move);
		pos->do_setup_move(mlist[index].move);
	}

	if (pos->is_mate()) {
		if (pos->side_to_move() == WHITE) {
			delete pos;
			return 0;
		}
		delete pos;
		return simcounter*1;
	}
	delete pos;
	return simcounter*0.5;
}

void MonteCarloTreeNode::normalUpdate(double value) {
	if (!whiteWins(totalValue) && !blackWins(totalValue)) {
		totalValue += value;
	}

	visits++;

	if (parent)
		parent->normalUpdate(value);
}

void MonteCarloTreeNode::updateKnownWin(double value, bool whiteToMove) {
	visits++;

	if (! ( (whiteToMove && whiteWins(value) && totalValue > value)
			|| (!whiteToMove && blackWins(value) && totalValue < value)))
		totalValue = value;

	if (!parent)
		return;

	// If the side to move is proven losing, the parent node is proven winning
	if ((!whiteToMove && whiteWins(value))) {
		parent->updateKnownWin(value, !whiteToMove);
		return;
	}

	if ((whiteToMove && blackWins(value))) {
		parent->updateKnownWin(value, !whiteToMove);
		return;
	}

	//The parent's node is a proven losing, if all its children are proven winning
	std::vector<MonteCarloTreeNode*>::iterator child;
	bool parentKnownLoss = true;
	int farthestLoss = 0;
	if (blackWins(value))
		farthestLoss = BLACK_MATES_IN_ONE;
	else if (whiteWins(value))
		farthestLoss = WHITE_MATES_IN_ONE;

	if (parent->children.size() == parent->maxMoves) {
		for (child = parent->children.begin(); child < parent->children.end(); child++) {
			if ((whiteWins(value) && !whiteWins((*child)->totalValue))
					|| (blackWins(value) && !blackWins((*child)->totalValue))) {
				parentKnownLoss = false;
				break;
			}
			else if (abs((*child)->totalValue) < abs(farthestLoss)) {
				farthestLoss = (*child)->totalValue;
			}
		}
	} else {
		parentKnownLoss = false;
	}

	if (parentKnownLoss) {
		if (!whiteToMove)
			parent->updateKnownWin(farthestLoss + 1, false);
		else
			parent->updateKnownWin(farthestLoss - 1, true);
	}
	else {
		if (blackWins(value))
			parent->normalUpdate(0);
		else if (whiteWins(value))
			parent->normalUpdate(1);
	}
}

void MonteCarloTreeNode::update(double value, Position * root) {
	if (value < 0 || value > 1) {
		bool whiteToMove;
		if (movesFromRoot().size() % 2 == 0)
			whiteToMove = (root->side_to_move() == WHITE) ? true : false;
		else
			whiteToMove = (root->side_to_move() == WHITE) ? false : true;
		updateKnownWin(value, whiteToMove);
	} else {
		this->normalUpdate(value);
	}
}

MonteCarloTreeNode * MonteCarloTreeNode::bestChild() {
	int maxVisits = 0;
	MonteCarloTreeNode * curBestChild = children[0];
	for (unsigned int i = 0; i < children.size(); i++) {
		if (children[i]->visits > maxVisits) {
			maxVisits = children[i]->visits;
			curBestChild = children[i];
		}
	}
	return curBestChild;
}

bool cmp_treeNode_ptrs(MonteCarloTreeNode * a, MonteCarloTreeNode * b) {
	return a->visits > b->visits;
}

void MonteCarloTreeNode::printMultiPv(int depth, int iterations, int searchTime, bool whiteToMove, int UCIMultiPV) {
	std::vector<MonteCarloTreeNode*> sortedChilds;

	std::vector<MonteCarloTreeNode*>::iterator child;
	for (child = children.begin(); child < children.end(); child++) {
		sortedChilds.push_back(*child);
	}
	std::sort(sortedChilds.begin(), sortedChilds.end(), cmp_treeNode_ptrs);

	child = sortedChilds.begin();
  for (int i = 0; i < Min(UCIMultiPV, (int)sortedChilds.size()); i++) {
      std::cout << (*child)->pv_info_to_uci(depth, iterations, searchTime, whiteToMove, i) << std::endl;
      child++;
	}

  sortedChilds.clear();
}

std::string MonteCarloTreeNode::pv_info_to_uci(int depth, unsigned long int iterations, int searchTime, bool whiteToMove, int multipv) {
	double score = (totalValue / visits);
	if (!whiteToMove)
		score = 1 - score;

	if (whiteWins(totalValue)) {
		score = value_mate_in(WHITE_MATES_IN_ONE - totalValue + 1);
		if (!whiteToMove)
			score = -score;
	}

	if (blackWins(totalValue)) {
		score = -value_mate_in(WHITE_MATES_IN_ONE + totalValue + 1);
		if (!whiteToMove)
			score = -score;
	}

	std::stringstream s;
	s.precision(0);
	s.setf(s.fixed);
	s
	<< "info"
	<< " depth " << depth
	<< " multipv " << multipv+1
	<< " score cp " <<  1000 * score - 500
	<< " nodes " << iterations;

	if (searchTime > 0)
		s << " nps " << 1000 * iterations / searchTime;

	s
	<< " time " << searchTime
	<< " pv " << move_to_uci(lastMove, false) << " ";

	MonteCarloTreeNode * cur = this;
	while (cur->children.size()>0) {
		cur=cur->bestChild();
		s << " " << move_to_uci(cur->lastMove, false);
	}
	return s.str();
}
