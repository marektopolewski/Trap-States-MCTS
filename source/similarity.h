#include "position.h"

#ifndef SIMILARITY_H_
#define SIMILARITY_H_

enum SimMethod {
    CONSTANT,
    DEPTH_BREADTH,
    INFL_PIECES,
    LEGAL_MOVES,
    REC_LEGAL_MOVES,
    EXPANDABLE_STATES,
    REC_EXPANDABLE_STATES
};

/** Constants **/
const double DEFAULT_SIM    = 1.0,   // default similarity value (in case of error, insufficient data, etc.)
             CONST_SIM      = 0.5,   // predefined similarity value for @similarity<CONSTANT>()
             DEPTH_WEIGHT   = 1,     // weight factor of tree depth used for @similarity<DEPTH_BREADTH>()
             BREADTH_WEIGHT = 1,     // weight factor of tree breadth used for @similarity<DEPTH_BREADTH>()
             ACC_THRESHOLD  = 0.6,   // threshold for accepting a recursively expanded mismatch in @similarity<REC_*>()
             REC_INCREMENT  = 1;     // value to increment the intersection if mismatch accepted in @similarity<REC_*>()

/**
 * Calculate the similarity measure between two arbitrary board positions. If the reference position is not defined
 * then the default similairty value is returned. For applicable templates see @file similairty.h.
 * @param curPos current board state
 * @param prevPos reference board state
 * @return similarity of curPos and prevPos (real number between 0 and 1 inclusive)
 */
template<SimMethod>
double similarity(Position* curPos, Position* prevPos);

/**
 * Wrapper method that allows to compute a similarity between 2 FEN-encoded board positions.
 * @tparam simMethod function used to compute the similairty
 * @param curFen current board state as FEN string
 * @param prevFen reference board state as FEN string
 * @return similarity between curPos and prevPos
 */
double similarity(std::string curFen, std::string prevFen, SimMethod method);

#endif /* SIMILARITY_H_ */
