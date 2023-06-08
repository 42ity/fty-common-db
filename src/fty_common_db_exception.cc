/*  =========================================================================
    fty_common_db_exception - Exception classs for DB errors

    Copyright (C) 2014 - 2020 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
*/

/*
@header
    fty_common_db_exception - Exception classs for DB errors
@discuss
@end
*/

#include "fty_common_db_exception.h"
#include <cxxtools/serializationinfo.h>
#include <sstream>
#include <fty_common_json.h>

namespace fty {
// Public
void CommonException::throwCommonException(const std::string& data)
{
    // get the serializationInfo
    cxxtools::SerializationInfo si;
    JSON::readFromString(data, si);


    // extract the error code, the message and the extra data
    int                         status       = 0;
    uint8_t                     errorType    = 0;
    uint8_t                     errorSubtype = 0;
    std::string                 whatArg;
    cxxtools::SerializationInfo extraData;

    try {
        si.getMember("status") >>= status;
        si.getMember("errorType") >>= errorType;
        si.getMember("errorSubtype") >>= errorSubtype;
        si.getMember("whatArg") >>= whatArg;
        extraData = si.getMember("extraData");
    } catch (...) {
    }

    throw CommonException(status, static_cast<ErrorType>(errorType), static_cast<ErrorSubtype>(errorSubtype), whatArg);
}

CommonException::CommonException(int status, ErrorType errorType, ErrorSubtype errorSubtype, const std::string& whatArg)
    : m_status(status)
    , m_errorType(errorType)
    , m_errorSubtype(errorSubtype)
    , m_whatArg(whatArg)
{
}

const char* CommonException::what() const noexcept
{
    return m_whatArg.c_str();
}

std::string CommonException::toJson() const
{
    cxxtools::SerializationInfo si;
    si <<= *(this);

    return JSON::writeToString(si, true);
}

void operator<<=(cxxtools::SerializationInfo& si, const CommonException& exception)
{
    si.addMember("status") <<= exception.m_status;
    si.addMember("errorType") <<= uint8_t(exception.m_errorType);
    si.addMember("errorSubtype") <<= uint8_t(exception.m_errorSubtype);
    si.addMember("whatArg") <<= exception.m_whatArg;
    exception.fillSerializationInfo(si.addMember("extraData"));
}

// Protected
void CommonException::fillSerializationInfo(cxxtools::SerializationInfo& /*si*/) const
{
}

} // namespace fty
