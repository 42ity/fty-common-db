cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

########################################################################################################################

project(fty_common_db
    VERSION 1.0.0
    DESCRIPTION "Provides common database tools for agents"
)

########################################################################################################################
find_package(fty-cmake PATHS ${CMAKE_BINARY_DIR}/fty-cmake)
########################################################################################################################

etn_target(shared ${PROJECT_NAME}
    PUBLIC_INCLUDE_DIR include
    PUBLIC
        fty_common_db_asset_delete.h
        fty_common_db_asset.h
        fty_common_db_asset_insert.h
        fty_common_db_asset_update.h
        fty_common_db_dbpath.h
        fty_common_db_defs.h
        fty_common_db_exception.h
        fty_common_db.h
        fty_common_db_uptime.h
        fty_common_db_connection.h
    SOURCES
        fty_common_db_asset.cc
        fty_common_db_asset_insert.cc
        fty_common_db_exception.cc
        fty_common_db_asset_delete.cc
        fty_common_db_asset_update.cc
        fty_common_db_dbpath.cc
        fty_common_db_uptime.cc
        fty_common_db_connection.cc
    USES
        czmq
        cxxtools
        fty_common_logging
        fty_common
        tntdb
    USES_PUBLIC
        fty-utils
)

set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})

########################################################################################################################

etn_test_target(${PROJECT_NAME}
    SOURCES
        test/main.cpp
    USES
        pthread
    SUBDIR
        test
)

########################################################################################################################

