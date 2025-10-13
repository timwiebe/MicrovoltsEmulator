project(AuthServer LANGUAGES CXX)


file(GLOB_RECURSE AUTH_SOURCES CONFIGURE_DEPENDS 
	${CMAKE_CURRENT_LIST_DIR}/*.cpp
)

add_executable(AuthServer
    ${AUTH_SOURCES}
)

target_include_directories(AuthServer PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(AuthServer PUBLIC
        Common
        unofficial::mariadb-connector-cpp::mariadbcpp
)

if (WIN32)
    target_link_libraries(AuthServer PUBLIC wsock32 ws2_32)
endif()


set_target_properties(AuthServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)

