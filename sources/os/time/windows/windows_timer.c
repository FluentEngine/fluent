#ifdef _WIN32

#include <windows.h>
#include "os/time/timer.h"

struct
{
	uint64_t start;
} windows_ticks;

FT_API void
ft_ticks_init()
{
	windows_ticks.start = GetTickCount64();
}

FT_API void
ft_ticks_shutdown()
{
}

FT_API uint64_t
ft_get_ticks()
{
	uint64_t now = GetTickCount64();
	return now - windows_ticks.start;
}

FT_API void
ft_nanosleep( int64_t milliseconds )
{
	milliseconds /= 1000000;

	HANDLE        timer;
	LARGE_INTEGER li;

	if ( !( timer = CreateWaitableTimer( NULL, true, NULL ) ) )
		return;

	li.QuadPart = -milliseconds;
	if ( !SetWaitableTimer( timer, &li, 0, NULL, NULL, false ) )
	{
		CloseHandle( timer );
		return;
	}

	WaitForSingleObject( timer, INFINITE );
	CloseHandle( timer );
}

#endif
