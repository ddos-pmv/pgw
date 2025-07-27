file(GLOB HEADERS "./*.h")
file(GLOB SOURCES "./*.cpp")

add_executable(pgw_server ${HEADERS} ${SOURCES})
# target_include_directories(ProteiPGW "./")
target_link_libraries(pgw_server 
    PRIVATE 
    pgw_utils
    concurrentqueue
    httplib::httplib)