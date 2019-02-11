#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ostream>
#include <algorithm>
#include <vector>

#include "ucioption.h"
#include "misc.h"
#include "search.h"
#include "montecarlotreenode.h"
#include "position.h"
#include "movegen.h"
#include "move.h"

#include "similarity.h"

using namespace std;

namespace {
	bool StopOnPonderhit, StopRequest, QuitRequest;
	bool whiteToMove;
	SearchLimits Limits;
	int depth;
	int UCIMultiPV;
	unsigned int iterations;

	// Trap Adaptiveness
	Position *prevPosBlanc, *prevPosNoir;

	// Time Management
	int searchStartTime;
	int thinkingTime;
	const int timeRate = 20;

	// current_search_time() returns the number of milliseconds which have passed
	// since the beginning of the current search.
	int current_search_time() {
		return get_system_time() - searchStartTime;
	}

	void uct_poll(MonteCarloTreeNode * root) {
		if (log(iterations) > depth)
			cout << "info depth " << ++depth << endl;

		root->printMultiPv(depth, iterations, current_search_time() / 1000, whiteToMove, UCIMultiPV);

		//  Poll for input
		if (input_available())
		{
			// We are line oriented, don't read single chars
			string command;

			if (!getline(cin, command) || command == "quit")
			{
				// Quit the program as soon as possible
				Limits.ponder = false;
				QuitRequest = StopRequest = true;
				return;
			}
			else if (command == "stop")
			{
				// Stop calculating as soon as possible, but still send the "bestmove"
				// and possibly the "ponder" token when finishing the search.
				Limits.ponder = false;
				StopRequest = true;
			}
			else if (command == "ponderhit")
			{
				// The opponent has played the expected move. GUI sends "ponderhit" if
				// we were told to ponder on the same move the opponent has played. We
				// should continue searching but switching from pondering to normal search.
				Limits.ponder = false;

				if (StopOnPonderhit)
					StopRequest = true;
			}
		}

		// Stop if we have no thinking time left
		if (!Limits.infinite && current_search_time() >= thinkingTime) {
			StopRequest = true;
			Limits.ponder = false;
		}
	}
}

bool uct(Position& pos, const SearchLimits& limits){
	 // Read UCI options
	UCIMultiPV = Options["MultiPV"].value<int>();

	// Initialize variables for the new search
	Limits = limits;
	whiteToMove = (pos.side_to_move()==WHITE);
	StopOnPonderhit = StopRequest = QuitRequest = false;
	depth = 10;
	iterations = 0;
	thinkingTime = Limits.time / timeRate;
	searchStartTime = get_system_time();

	MonteCarloTreeNode * root = new MonteCarloTreeNode(MOVE_NONE, NULL, VALUE_ZERO);
	MonteCarloTreeNode * selected0;
	MonteCarloTreeNode * selected1;
	MonteCarloTreeNode * expanded0;
	MonteCarloTreeNode * expanded1;
	double result, sim;

	if (whiteToMove) {
		sim = similarity<REC_LEGAL_MOVES>(&pos, prevPosBlanc);
		prevPosBlanc = new Position(pos,pos.thread());
	}
	else {
		sim = similarity<REC_LEGAL_MOVES>(&pos, prevPosNoir);
		prevPosNoir = new Position(pos,pos.thread());
	}

	while(!StopRequest) {
		selected0 = root->UCT_select(&pos);
		expanded0 = selected0->UCT_expand(&pos);
		selected1 = expanded0->UCT_select(&pos);
		expanded1 = selected1->UCT_expand(&pos);
		//selected2 = expanded1->UCT_select(&pos);
		//expanded2 = selected2->UCT_expand(&pos);
		result = expanded1->simulate(sim, &pos);
		expanded1->update(result, &pos);
		if (iterations++ % 1000 == 0)
			uct_poll(root);
	}

	root->printMultiPv(depth, iterations, current_search_time(), whiteToMove, UCIMultiPV);
	cout << "info string " << "sim=" << sim << endl;
	cout << "bestmove " << move_to_uci(root->bestChild()->lastMove, false) << endl;

	delete root;
	return !QuitRequest;
}

bool trapcheck(Move m) {
	MoveStack mlist[MAX_MOVES];
	MoveStack *last;
	if (whiteToMove) {
		last = generate<TRAP>(prevPosBlanc, mlist);
		for (MoveStack *i = mlist; i != last; i++) {
			if (m == i->move) return true;
		}
	}
	else {
		last = generate<TRAP>(prevPosNoir, mlist);
		for (MoveStack *i = mlist; i != last; i++) {
			if (m == i->move) return true;
		}
	}
	return false;
}
