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

#pragma once
#include <stdexcept>
#include <string>

namespace cxxtools {
class SerializationInfo;
}

namespace fty {
enum ErrorType : uint8_t
{
    UNKNOWN_ERR = 0,
    DB_ERR,
    BAD_INPUT,
    INTERNAL_ERR,
    LICENSING_ERR,
    REQUEST_PARAM_BAD_ERR,
    DATA_CONFLICT_ERR,
};

enum ErrorSubtype : uint8_t
{
    DB_ERROR_UNKNOWN  = 0,
    BAD_INPUT_UNKNOWN = 0,
    INTERNAL_UNKNOWN  = 0,
    DB_ERROR_INTERNAL,
    // Probably should be removed at some point and replaced with bad_input_err
    DB_ERROR_BADINPUT,
    DB_ERROR_NOTFOUND,
    DB_ERROR_NOTIMPLEMENTED,
    DB_ERROR_DBCORRUPTED,
    DB_ERROR_NOTHINGINSERTED,
    DB_ERROR_DELETEFAIL,
    DB_ERROR_CANTCONNECT,
    BAD_INPUT_WRONG_INPUT,
    BAD_INPUT_OUT_OF_BOUNDS,
    INTERNAL_NOT_IMPLEMENTED,
    LICENSING_GLOBAL_CONFIGURABILITY_DISABLED,
    LICENSING_POWER_DEVICES_COUNT_REACHED
};

class CommonException : public std::exception
{
public:
    explicit CommonException(int status, ErrorType errorType, ErrorSubtype errorSubtype, const std::string& whatArg);

    ~CommonException() override
    {
    }

    const char* what() const noexcept override;

    inline int getStatus() const
    {
        return m_status;
    }
    inline ErrorType getErrorType() const
    {
        return m_errorType;
    }
    inline ErrorSubtype getErrorSubtype() const
    {
        return m_errorSubtype;
    }

    // return error payload of the exception
    std::string toJson() const;

    friend void operator<<=(cxxtools::SerializationInfo& si, const CommonException& exception);

    [[noreturn]] static void throwCommonException(const std::string& data);

protected:
    virtual void fillSerializationInfo(cxxtools::SerializationInfo& si) const;

private:
    int          m_status;
    ErrorType    m_errorType;
    ErrorSubtype m_errorSubtype;
    std::string  m_whatArg;
};

void operator<<=(cxxtools::SerializationInfo& si, const CommonException& exception);

} // namespace fty
