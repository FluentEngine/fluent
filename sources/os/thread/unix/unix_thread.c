#if ( defined( __unix__ ) || defined( __unix ) ||                              \
      ( defined( __APPLE__ ) && defined( __MACH__ ) ) )
#include <pthread.h>
#include "os/thread/thread.h"

FT_API void
ft_thread_create( struct ft_thread* thread, ft_thread_fun fun, void* arg )
{
	FT_ASSERT( thread );

	pthread_t* t = malloc( sizeof( pthread_t ) );
	pthread_create( t, NULL, fun, arg );
	thread->handle = t;
}

FT_API void
ft_thread_destroy( struct ft_thread* thread )
{
	FT_ASSERT( thread );

	free( thread->handle );
}

FT_API void
ft_thread_join( struct ft_thread* thread )
{
	FT_ASSERT( thread );

	pthread_t* t = thread->handle;
	pthread_join( *t, NULL );
}

FT_API void
ft_mutex_create( struct ft_mutex* mtx )
{
	FT_ASSERT( mtx );

	pthread_mutex_t* m = malloc( sizeof( pthread_mutex_t ) );
	*m                 = ( pthread_mutex_t ) PTHREAD_MUTEX_INITIALIZER;
	mtx->handle        = m;
}

FT_API void
ft_mutex_destroy( struct ft_mutex* mtx )
{
	FT_ASSERT( mtx );

	free( mtx->handle );
}

FT_API bool
ft_mutex_try_lock( struct ft_mutex* mtx )
{
	FT_ASSERT( mtx );

	pthread_mutex_t* m = mtx->handle;
	return pthread_mutex_trylock( m ) == 0;
}

FT_API void
ft_mutex_unlock( struct ft_mutex* mtx )
{
	FT_ASSERT( mtx );

	pthread_mutex_t* m = mtx->handle;
	pthread_mutex_unlock( m );
}

#endif
