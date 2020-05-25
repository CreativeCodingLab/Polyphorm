#pragma once
#include <stdint.h>

#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

#define print_error(format, ...) print_error_with_location(format, __FILENAME__, __LINE__, __VA_ARGS__)

namespace logging
{
	// General purpose print
	void print(char *format, ...);

	// Prints error message along with filename and line number.
	// Example:
	// "ERROR in file %FILENAME on line %LINE_NUMBER: "
	// %ERROR_MESSAGE
	void print_error_with_location(char *format, char *filename, uint32_t line, ...);
}