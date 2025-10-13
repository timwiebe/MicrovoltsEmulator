project(Common LANGUAGES CXX)

file(GLOB_RECURSE COMMON_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
)

add_library(Common STATIC
    ${COMMON_SOURCES}
)

set_target_properties(Common PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/ExternalLibraries/CommonLib
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/ExternalLibraries/CommonLib
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/ExternalLibraries/CommonLib
    ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/ExternalLibraries/CommonLib
    ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_SOURCE_DIR}/ExternalLibraries/CommonLib

    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/ExternalLibraries/CommonLib
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/ExternalLibraries/CommonLib
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/ExternalLibraries/CommonLib
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/ExternalLibraries/CommonLib
    LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_SOURCE_DIR}/ExternalLibraries/CommonLib

    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/ExternalLibraries/CommonLib
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/ExternalLibraries/CommonLib
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/ExternalLibraries/CommonLib
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/ExternalLibraries/CommonLib
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_SOURCE_DIR}/ExternalLibraries/CommonLib
)

target_include_directories(Common
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_SOURCE_DIR}/ExternalLibraries
        ${CMAKE_SOURCE_DIR}/Common/include/Utils
)

target_link_libraries(Common
    PUBLIC   
        cryptopp::cryptopp
        unofficial::mariadb-connector-cpp::mariadbcpp
)

if (WIN32)
    target_link_libraries(Common PUBLIC wsock32 ws2_32)
endif()
