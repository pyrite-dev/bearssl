project("bearssl")
        kind("StaticLib")
        targetdir("lib/%{cfg.buildcfg}/%{cfg.platform}")
	includedirs({
		"src",
		"inc"
	})
	files("src/**.c")
