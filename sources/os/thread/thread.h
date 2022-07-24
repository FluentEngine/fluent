#pragma once

#include "base/base.h"

struct ft_thread
{
	void* handle;
};

struct ft_mutex
{
	void* handle;
};

FT_API void
ft_thread_create( struct ft_thread* thread,
                  void* ( *thread_fun )( void* ),
                  void* arg );

FT_API void
ft_thread_destroy( struct ft_thread* thread );

FT_API void
ft_thread_join( struct ft_thread* thread );

FT_API void
ft_mutex_create( struct ft_mutex* mtx );

FT_API void
ft_mutex_destroy( struct ft_mutex* mtx );

FT_API bool
ft_mutex_try_lock( struct ft_mutex* mtx );

FT_API void
ft_mutex_unlock( struct ft_mutex* mtx );
