project "stb"
    kind "StaticLib"
    language "C"

    includedirs {
        ".",
    }

    files {
		"stb_image.h",
        "stb.c"
    }
