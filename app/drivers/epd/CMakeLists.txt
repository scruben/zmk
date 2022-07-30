zephyr_include_directories(.)

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_IL0323N		il0323n.c)