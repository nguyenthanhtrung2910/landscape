#include <UnigineInit.h>
#include <UnigineEngine.h>

#include <cstdio>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "AppSystemLogic.h"
#include "AppWorldLogic.h"

#ifdef _WIN32
int wmain(int argc, wchar_t *argv[])
#else
int main(int argc, char *argv[])
#endif
{
#ifdef _WIN32
	// The Windows GUI subsystem does not attach stdout to the launching terminal.
	if (AttachConsole(ATTACH_PARENT_PROCESS))
	{
		FILE *terminal_output = nullptr;
		freopen_s(&terminal_output, "CONOUT$", "w", stdout);
	}
#endif

	// UnigineLogic
	AppSystemLogic system_logic;
	AppWorldLogic world_logic;

	// init engine
	Unigine::EnginePtr engine(argc, argv);

	// enter main loop
	engine->main(&system_logic, &world_logic);

	return 0;
}
