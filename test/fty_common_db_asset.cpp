#include <catch2/catch.hpp>
#include "fty_common_db.h"
#include <iostream>

static bool runWithPrivileges{false}; // run /w db access privileges?

TEST_CASE("get_status_from_db_helper #1")
{
    const std::string UNKNOWN{"unknown"};

    //pass nullptr as const std::string& argument throw an exception
    // "basic_string::_M_construct null not valid"
    {
        bool exception = false;
        try {
            std::string result = DBAssets::get_status_from_db_helper(nullptr);
        }
        catch (const std::exception& e) {
            std::cout << "Exception reached: " << e.what() << std::endl;
            exception = true;
        }
        CHECK(exception);
    }

    if (runWithPrivileges) {
        std::string result = DBAssets::get_status_from_db_helper("");
        CHECK(result == UNKNOWN);
    }

    if (runWithPrivileges) {
        std::string result = DBAssets::get_status_from_db_helper("fake");
        CHECK(result == UNKNOWN);
    }
}
