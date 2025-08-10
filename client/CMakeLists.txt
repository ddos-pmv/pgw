file(GLOB HEADERS "./*.h")
file(GLOB SOURCES "./*.cpp")

add_executable(pgw_client ${SOURCES})

target_link_libraries(pgw_client PRIVATE pgw_utils)