#zephyr_include_directories(.)
#zephyr_library()
#zephyr_library_sources(il0323n.c)
zephyr_sources_ifdef(CONFIG_IL0323N		il0323n.c)