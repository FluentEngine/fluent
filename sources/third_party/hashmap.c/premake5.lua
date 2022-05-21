project "hashmap_c"
    kind "StaticLib"
    language "C"

    includedirs {
        "../"
    }

    files {
        "hashmap.h",
        "hashmap.c"
    }