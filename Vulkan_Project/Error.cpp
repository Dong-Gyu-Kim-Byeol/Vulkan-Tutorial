#include "Error.h"

void Error(const char * const sorceFileName, const char* const functionName, const char* const string)
{
	// 파일에 쓰기로 변경해야 함
	std::cout << "\nError - " << sorceFileName << ".cpp : " << functionName << " : " << string << std::endl;
	std::exit(EXIT_FAILURE);
}
