#pragma once

#include "../../base/base.h"
#include "window.h"

FT_DECLARE_FUNCTION_POINTER( void, ft_destroy_window, struct ft_window* );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_window_set_resize_callback,
                             struct ft_window*         window,
                             ft_window_resize_callback cb );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_window_get_size,
                             const struct ft_window*,
                             uint32_t*,
                             uint32_t* );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_window_get_framebuffer_size,
                             const struct ft_window*,
                             uint32_t*,
                             uint32_t* );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_window_show_cursor,
                             struct ft_window*,
                             bool );

FT_DECLARE_FUNCTION_POINTER( bool,
                             ft_window_should_close,
                             const struct ft_window* );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_window_get_vulkan_instance_extensions,
                             const struct ft_window*,
                             uint32_t*,
                             const char** );

FT_DECLARE_FUNCTION_POINTER( void,
                             ft_window_create_vulkan_surface,
                             const struct ft_window*,
                             VkInstance,
                             const struct VkAllocationCallbacks*,
                             VkSurfaceKHR* );

FT_DECLARE_FUNCTION_POINTER( void, ft_poll_events );

FT_DECLARE_FUNCTION_POINTER( const uint8_t*, ft_get_keyboard_state, uint32_t* );

FT_DECLARE_FUNCTION_POINTER( uint32_t, ft_get_mouse_state, int32_t*, int32_t* );
