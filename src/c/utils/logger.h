#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

/**
 * @brief Logs message and flushes the stream. This is necessary for the logs
 * to be displayed when run in docker containers.
 * 
 * @param out Output stream.
 * @param str Format string.
 * @param ... Arguments to insert into format.
 */
void log_msg(FILE* out, const char* str, ...);

#endif
