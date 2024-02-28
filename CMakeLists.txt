project(api_8367b)

file(GLOB_RECURSE SRCS "api_8367b/*.c")

# rtl8367b_asicdrv.c里定义这个，读写寄存器就只用实现setReg()和getReg()了
add_definitions(-DEMBEDDED_SUPPORT)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/api_8367b
)
add_executable(${PROJECT_NAME}
    ${SRCS}
    main.c
)
