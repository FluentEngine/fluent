#ifdef _WIN32

#include <windows.h>
#include "os/thread/thread.h"

FT_API void
ft_thread_create( struct ft_thread* thread, ft_thread_fun fun, void* arg )
{
	FT_ASSERT( thread );
	FT_ASSERT( fun );

	thread->handle = CreateThread( NULL, 0, fun, arg, 0, NULL );
}

FT_API void
ft_thread_destroy( struct ft_thread* thread )
{
	FT_ASSERT( thread );

	DWORD exit_code;
	TerminateThread( thread->handle,
	                 GetExitCodeThread( thread->handle, &exit_code ) );
	CloseHandle( thread->handle );
}

FT_API void
ft_thread_join( struct ft_thread* thread )
{
	FT_ASSERT( thread );

	WaitForSingleObject( thread->handle, INFINITE );
}

FT_API void
ft_mutex_create( struct ft_mutex* mtx )
{
	FT_ASSERT( mtx );

	mtx->handle = CreateMutex( NULL, false, L"mtx" );
}

FT_API void
ft_mutex_destroy( struct ft_mutex* mtx )
{
	FT_ASSERT( mtx );

	CloseHandle( mtx->handle );
}

FT_API bool
ft_mutex_try_lock( struct ft_mutex* mtx )
{
	FT_ASSERT( mtx );

	return WaitForSingleObject( mtx->handle, 0 ) == WAIT_OBJECT_0;
}

FT_API void
ft_mutex_unlock( struct ft_mutex* mtx )
{
	ReleaseMutex( mtx->handle );
}

#endif
