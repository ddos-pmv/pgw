file(GLOB_RECURSE PGW_SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE PGW_HEADERS CONFIGURE_DEPENDS "include/pgw_utils/*.h")

add_library(pgw_utils STATIC ${PGW_SOURCES} ${PGW_HEADERS})

target_include_directories(pgw_utils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pgw_utils
    PUBLIC
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        Threads::Threads
)