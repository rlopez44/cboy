#ifndef LOG_H_
#define LOG_H_

#ifdef DEBUG
#include <SDL_log.h>
#include "cboy/gameboy.h"

#define ENABLE_DEBUG_LOGS() SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_VERBOSE)

/* credit to https://github.com/sysprog21/jitboy for these macros */
#define LOG_DEBUG(...) SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)
#define LOG_ERROR(...) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)
#define LOG_INFO(...)  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__)

// dump the Game Boy's memory contents
void dump_memory(gameboy *gb);

#else
#include <stdio.h>
#define LOG_DEBUG(...)
#define LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)
#define LOG_INFO(...)  printf(__VA_ARGS__)

#endif /* DEBUG */

#endif /* LOG_H_ */
