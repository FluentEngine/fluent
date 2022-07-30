#pragma once

#include "math/linear.h"
#include "model_loader.h"

FT_INLINE void
apply_animation_channel( float4x4                           r,
                         float                              current_time,
                         const struct ft_animation_channel* channel )
{
	float3 translation;
	quat   rotation;
	float3 scale;

	float4x4_decompose( translation, rotation, scale, r );

	struct ft_animation_sampler* sampler = channel->sampler;

	if ( sampler->frame_count < 2 )
	{
		return;
	}

	uint32_t previous_frame = 0;
	uint32_t next_frame     = 0;

	float interpolation_value = 0.0f;

	for ( uint32_t f = 0; f < sampler->frame_count; f++ )
	{
		if ( sampler->times[ f ] >= current_time )
		{
			next_frame = f;
			break;
		}
	}

	if ( next_frame == 0 )
	{
		previous_frame = 0;
	}
	else
	{
		previous_frame = next_frame - 1;
		interpolation_value =
		    ( current_time - sampler->times[ previous_frame ] ) /
		    ( sampler->times[ next_frame ] - sampler->times[ previous_frame ] );
	}

	switch ( channel->transform_type )
	{
	case FT_TRANSFORM_TYPE_TRANSLATION:
	{
		float3* translations = ( float3* ) sampler->values;
		float3_lerp( translation,
		             translations[ previous_frame ],
		             translations[ next_frame ],
		             interpolation_value );
		float4x4_compose( r, translation, rotation, scale );
		break;
	}
	case FT_TRANSFORM_TYPE_ROTATION:
	{
		quat* quats = ( quat* ) sampler->values;
		slerp( rotation,
		       quats[ previous_frame ],
		       quats[ next_frame ],
		       interpolation_value );
		float4x4_compose( r, translation, rotation, scale );
		break;
	}
	case FT_TRANSFORM_TYPE_SCALE:
	{
		float3* scales = ( float3* ) sampler->values;
		float3_lerp( scale,
		             scales[ previous_frame ],
		             scales[ next_frame ],
		             interpolation_value );
		break;
	}
	case FT_TRANSFORM_TYPE_WEIGHTS:
	{
		FT_WARN( "not implemented yet" );
		break;
	}
	}
}

FT_INLINE void
apply_animation( float4x4*                  transforms,
                 float                      current_time,
                 const struct ft_animation* animation )
{
	current_time = fmod( current_time, animation->duration );

	// TODO: animation channels should be applied in correct order
	for ( int32_t ch = animation->channel_count - 1; ch >= 0; ch-- )
	{
		struct ft_animation_channel* channel = &animation->channels[ ch ];

		apply_animation_channel( transforms[ channel->target ],
		                         current_time,
		                         channel );
	}
}
