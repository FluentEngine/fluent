#pragma once

#include "base/base.h"

struct ft_thread
{
	void*    handle;
	uint64_t thread_id;
};

struct ft_mutex
{
	void* handle;
};

typedef uint32_t ( *ft_thread_fun )( void* );

FT_API void
ft_thread_create( struct ft_thread* thread, ft_thread_fun fun, void* arg );

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
