#include "Error.h"

void Error(const char * const sorceFileName, const char* const functionName, const char* const string)
{
	// ���Ͽ� ����� �����ؾ� ��
	std::cout << "\nError - " << sorceFileName << ".cpp : " << functionName << " : " << string << std::endl;
	std::exit(EXIT_FAILURE);
}
