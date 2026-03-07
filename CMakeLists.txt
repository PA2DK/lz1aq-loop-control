cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr)
project(lz1aq_loop_control)

zephyr_linker_sources(SECTIONS sections-rom.ld)
zephyr_linker_section(NAME http_resource_desc_lz1aq_loop_control
                      KVMA RAM_REGION GROUP RODATA_REGION)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

foreach(web_resource
    index.html
    main.js
)
    generate_inc_file_for_target(app
        src/www/${web_resource}
        ${gen_dir}/${web_resource}.gz.inc
        --gzip
    )
endforeach()

target_sources(app PRIVATE src/main.c)