#define SAMPLE_NAME "00_minimal"
#include "../common/sample.hpp"

void
init_sample()
{
}

void
resize_sample( u32, u32 )
{
}

void
update_sample( CommandBuffer*, f32 )
{
	u32 image_index = begin_frame();
	end_frame( image_index );
}

void
shutdown_sample()
{
}
