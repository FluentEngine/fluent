#pragma once

#include "base/base.h"

#ifdef __linux__
#include <pthread.h>
#endif

struct ft_thread
{
#ifdef __linux__
	pthread_t handle;
#endif
};

struct ft_mutex
{
#ifdef __linux__
	pthread_mutex_t handle;
#endif
};

FT_API void
ft_thread_create( struct ft_thread* thread,
                  void* ( *thread_fun )( void* ),
                  void* arg );

FT_API void
ft_thread_join( struct ft_thread* thread );

FT_API void
ft_mutex_init( struct ft_mutex* mtx );

FT_API bool
ft_mutex_try_lock( struct ft_mutex* mtx );

FT_API void
ft_mutex_unlock( struct ft_mutex* mtx );
