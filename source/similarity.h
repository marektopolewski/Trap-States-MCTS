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

const std::string EXIT_CMD   = "exit",  // commands used for similairty testing
                  MANUAL_CMD = "man",
                  AUTO_CMD   = "auto";
const double CANCEL_SIM  = -999,        // impossible similarity value used to indicate termination
             INVALID_FEN = -998;        // impossible similarity value used to indicate invalid FEN positions
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
 * Method called from an UCI command to begin similairty measure testing
 */
void similarityTest();

#endif /* SIMILARITY_H_ */
