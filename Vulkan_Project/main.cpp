
#ifdef _DEBUG
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
	#define new new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
	// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
	// allocations to be of _CLIENT_BLOCK type
#endif

#include "app.h"

int main()
{
#ifdef _DEBUG
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	VkApp app;
	GLFWwindow* window = app.GetWindow();

	std::thread mainLoop([&]() { app.Run(); });
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}


	mainLoop.join();

	return EXIT_SUCCESS;
}