#ifndef LOG_H_
#define LOG_H_

#ifdef DEBUG
#include <SDL_log.h>
#include "cboy/gameboy.h"

#define ENABLE_DEBUG_LOGS() do {SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_VERBOSE);} while (0)

/* credit to https://github.com/sysprog21/jitboy for these macros */
#define LOG_DEBUG(...) do {SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__);} while (0)
#define LOG_ERROR(...) do {SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__);} while (0)
#define LOG_INFO(...)  do {SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, __VA_ARGS__);} while (0)

// print out the current CPU register contents
void print_registers(gameboy *gb);

#else
#include <stdio.h>
#define LOG_DEBUG(...) do {} while (0)
#define LOG_ERROR(...) do {fprintf(stderr, __VA_ARGS__);} while (0)
#define LOG_INFO(...)  do {printf(__VA_ARGS__);} while (0)

#endif /* DEBUG */

#endif /* LOG_H_ */
