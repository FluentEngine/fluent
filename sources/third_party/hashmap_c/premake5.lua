project "hashmap_c"
    kind "StaticLib"
    language "C"

    includedirs {
        "../"
    }

    files {
        "hashmap_c.h",
        "hashmap_c.c"
    }
