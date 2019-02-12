#include "position.h"
#include "search.h"
#include "move.h"

#ifndef UCTSEARCH_H_
#define UCTSEARCH_H_

extern bool uct(Position& pos, const SearchLimits& limits);
extern bool trapcheck(Move m);

#endif /* UCTSEARCH_H_ */
