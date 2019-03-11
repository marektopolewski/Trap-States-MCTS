#include "position.h"

#ifndef SIMILARITY_TEST_H_
#define SIMILARITY_TEST_H_

/**
 * Commands used for similairty testing
 */
const std::string EXIT_CMD   = "exit",
                  MANUAL_CMD = "man",
                  CHILD_CMD  = "child",
                  AUTO_CMD   = "auto";

const double INVALID_FEN = -998;        // impossible similarity value used to indicate invalid FEN positions

/**
 * Method called from an UCI command to begin similairty measure testing
 */
void similarityTest();
bool isTrap(Position *pos);

#endif /* SIMILARITY_TEST_H_ */
