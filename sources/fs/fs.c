#include <stb/stb_image.h>
#include "fs.h"

void*
ft_read_file_binary( const char* filename, uint64_t* size )
{
	FILE* file = fopen( filename, "rb" );
	if ( !file )
	{
		FT_WARN( "failed to open file %s", filename );
		return NULL;
	}
	uint64_t res = fseek( file, 0, SEEK_END );
	FT_ASSERT( res == 0 );
	*size = ftell( file );
	res   = fseek( file, 0, SEEK_SET );
	FT_ASSERT( res == 0 );
	void* file_data = malloc( *size );
	fread( file_data, *size, 1, file );
	res = fclose( file );
	FT_ASSERT( res == 0 );
	return file_data;
}

void
ft_free_file_data( void* data )
{
	free( data );
}

void*
ft_read_image_from_file( const char* filename,
                         uint32_t*   width,
                         uint32_t*   height )
{
	FILE* file = fopen( filename, "rb" );
	if ( !file )
	{
		FT_WARN( "failed to open file %s", filename );
		return NULL;
	}

	int32_t w;
	int32_t h;
	int32_t ch;

	void* image = stbi_loadf_from_file( file, &w, &h, &ch, 4 );
	FT_ASSERT( image );

	*width  = w;
	*height = h;

	return image;
}

void
ft_free_image_data( void* data )
{
	stbi_image_free( data );
}
