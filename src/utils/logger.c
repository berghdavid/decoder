#include <stdio.h>
#include <stdarg.h>
#include "logger.h"

void log_msg(FILE* out, const char* str, ...)
{
	va_list	args;
	int	len;

	va_start(args, str);
	len = vsnprintf(NULL, 0, str, args);
	va_end(args);
	if (len < 0) {
		return;
	}

	char msg[len + 1];
	va_start(args, str);
	vsnprintf(msg, len + 1, str, args);
	va_end(args);

	fprintf(out, "%s", msg);
	fflush(out);
}
