add_executable(pgw_client main.cpp client_config.h)


target_link_libraries(pgw_client PRIVATE pgw_utils)