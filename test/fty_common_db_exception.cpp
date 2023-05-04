#include <catch2/catch.hpp>
#include <iostream>
#include "fty_common_db_exception.h"

TEST_CASE("common_db_exception")
{
    try {
        fty::CommonException::throwCommonException(R"({"status":0,"errorType":0,"errorSubtype":0,"whatArg":"test error","extraData":null})");
    } catch(const fty::CommonException& e) {
        CHECK(e.getStatus() == 0);
        CHECK(e.getErrorType() == fty::ErrorType::UNKNOWN_ERR);
        CHECK(e.getErrorSubtype() == fty::ErrorSubtype::DB_ERROR_UNKNOWN);
    }
}