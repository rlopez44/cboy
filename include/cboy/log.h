#ifndef LOG_H_
#define LOG_H_

#ifdef DEBUG
#include "cboy/gameboy.h"

// dump the Game Boy's memory contents
void dump_memory(gameboy *gb);
#endif /* DEBUG */

#endif /* LOG_H_ */
