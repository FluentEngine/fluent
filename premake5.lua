workspace "fluent-engine"
    targetdir "bin/%{prj.name}"
    objdir "build/%{prj.name}"
    location "build"
    configurations { "release", "debug" }

    include("fluent-engine.lua")