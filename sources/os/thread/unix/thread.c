#ifdef __linux__
#include <pthread.h>
#include "os/thread/thread.h"

FT_API void
ft_thread_create( struct ft_thread* thread,
                  void* ( *thread_fun )( void* ),
                  void* arg )
{
	FT_ASSERT( thread );

	pthread_t t;
	pthread_create( &t, NULL, thread_fun, arg );
	thread->handle = t;
}

FT_API void
ft_thread_join( struct ft_thread* thread )
{
	FT_ASSERT( thread );

	pthread_t t = thread->handle;
	pthread_join( t, NULL );
}

FT_API void
ft_mutex_init( struct ft_mutex* mtx )
{
	FT_ASSERT( mtx );

	mtx->handle = ( pthread_mutex_t ) PTHREAD_MUTEX_INITIALIZER;
}

FT_API bool
ft_mutex_try_lock( struct ft_mutex* mtx )
{
	FT_ASSERT( mtx );

	return pthread_mutex_trylock( &mtx->handle ) == 0;
}

FT_API void
ft_mutex_unlock( struct ft_mutex* mtx )
{
	FT_ASSERT( mtx );

	pthread_mutex_unlock( &mtx->handle );
}

#endif