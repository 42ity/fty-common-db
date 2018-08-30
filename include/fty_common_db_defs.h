/*  =========================================================================
    fty-common-db - Provides common database tools for agents

    Copyright (C) 2014 - 2018 Eaton

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

#ifndef FTY_COMMON_DB_DEFS_H_INCLUDED
#define FTY_COMMON_DB_DEFS_H_INCLUDED

#include <functional>
#include <inttypes.h>
#include <tntdb.h>
#include <czmq.h>

#ifdef __cplusplus
#define INPUT_POWER_CHAIN     1

typedef std::function<void(const tntdb::Row&)> row_cb_f ;

template <typename T>
struct db_reply{
    int status; // ok/fail
    int errtype;
    int errsubtype;
    uint64_t rowid;  // insert/update or http error code if status == 0
    uint64_t affected_rows; // for update/insert/delete
    std::string msg;
    zhash_t *addinfo;
    T item;
};

typedef db_reply<uint64_t> db_reply_t;

inline db_reply_t db_reply_new() {
    db_reply_t val;
    val.status = 1;
    val.errtype = 0;
    val.errsubtype = 0;
    val.rowid = 0;
    val.affected_rows = 0;
    val.msg = "";
    val.addinfo = NULL;
    val.item = 0;

    return val;
}

template <typename T>
inline db_reply<T> db_reply_new(T& item) {
    db_reply<T> val;
    val.status = 1;
    val.errtype = 0;
    val.errsubtype = 0;
    val.rowid = 0;
    val.affected_rows = 0;
    val.msg = "";
    val.addinfo = NULL;
    val.item = item;
    return val;
}

struct db_web_basic_element_t {
    uint32_t    id;
    std::string name;
    std::string status;
    uint16_t    priority;
    uint16_t    type_id;
    std::string type_name;
    uint32_t    parent_id;
    uint16_t    parent_type_id;
    uint16_t    subtype_id;
    // TODO location
    std::string subtype_name;
    std::string asset_tag;
    std::string parent_name;
};

typedef struct _asset_link
{
    uint32_t    src;     //!< id of src element
    uint32_t    dest;    //!< id of dest element
    char        *src_out; //!< outlet in src
    char        *dest_in; //!< inlet in dest
    uint16_t    type;    //!< link type id
} link_t;

struct db_tmp_link_t {
    uint32_t         src_id;
    uint32_t         dest_id;
    std::string      src_name;
    std::string      src_socket;
    std::string      dest_socket;
};

struct db_web_element_t{
    db_web_basic_element_t basic;
    std::map <uint32_t, std::string> groups;
    std::vector <db_tmp_link_t> powers;
    std::map <std::string, std::pair<std::string, bool> > ext;
    std::vector <std::tuple <uint32_t, std::string, std::string, std::string>> parents;        // list of parents (id, name)
};

struct db_a_elmnt_t {
    uint32_t         id;
    std::string      name;
    std::string      status;
    uint32_t         parent_id;
    uint16_t         priority;
    uint16_t         type_id;
    uint16_t         subtype_id;
    std::string      asset_tag;
    std::map <std::string, std::string> ext;

    db_a_elmnt_t () :
        id{},
        name{},
        status{},
        parent_id{},
        priority{},
        type_id{},
        subtype_id{},
        asset_tag{},
        ext{}
    {}

    db_a_elmnt_t (
        uint32_t         id,
        std::string      name,
        std::string      status,
        uint32_t         parent_id,
        uint8_t          priority,
        uint16_t         type_id,
        uint16_t         subtype_id,
        std::string      asset_tag) :

        id(id),
        name(name),
        status(status),
        parent_id(parent_id),
        priority(priority),
        type_id(type_id),
        subtype_id(subtype_id),
        asset_tag(asset_tag),
        ext{}
    {}
};

// FIXME: mapping is taken from fty-common-rest and should be extracted into a common library
//! Possible error types
enum errtypes {
    //! First error should be UNKNOWN as it maps to zero and zero is weird
    UNKNOWN_ERR,
    DB_ERR,
    BAD_INPUT,
    INTERNAL_ERR,
    LICENSING_ERR,
    REQUEST_PARAM_BAD_ERR,
    DATA_CONFLICT_ERR,
};

//! Constants for database errors
enum db_err_nos {
    //! First error should be UNKNOWN as it maps to zero and zero is weird
    DB_ERROR_UNKNOWN,
    DB_ERROR_INTERNAL,
    // Probably should be removed at some point and replaced with bad_input_err
    DB_ERROR_BADINPUT,
    DB_ERROR_NOTFOUND,
    DB_ERROR_NOTIMPLEMENTED,
    DB_ERROR_DBCORRUPTED,
    DB_ERROR_NOTHINGINSERTED,
    DB_ERROR_DELETEFAIL,
    DB_ERROR_CANTCONNECT,
};

//! Constants for bad input type of error
enum bad_input_err {
    //! First error should be UNKNOWN as it maps to zero and zero is weird
    BAD_INPUT_UNKNOWN,
    BAD_INPUT_WRONG_INPUT,
    BAD_INPUT_OUT_OF_BOUNDS,
};

//! Constants for internal errors
enum internal_err {
    //! First error should be UNKNOWN as it maps to zero and zero is weird
    INTERNAL_UNKNOWN,
    INTERNAL_NOT_IMPLEMENTED
};

#endif
#endif
