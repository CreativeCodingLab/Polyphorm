#include "logging.h"
#include <cstdarg>
#include <stdio.h>
#include <string>

void logging::print(char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int32_t chars_written = vprintf(fmt, args);
	va_end(args);
	printf("\n");
}

void logging::print_error_with_location(char *format, char *filename, uint32_t line, ...)
{
	printf("ERROR in file %s on line %d: ", filename, line);
	
	va_list args;
	va_start(args, line);
	logging::print(format, args);
	va_end(args);
}
