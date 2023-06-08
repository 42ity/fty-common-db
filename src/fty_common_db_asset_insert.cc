/*  =========================================================================
    fty_common_db_asset_insert - Functions inserting assets to database.

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
    fty_common_db_asset_insert - Functions inserting assets to database.
@discuss
@end
*/

#include <fty_log.h>
#include "fty_common_db.h"
#include <fty_common.h>
#include <fty_common_db_dbpath.h>
#include <fty_common_db_defs.h>
#include <fty_common_macros.h>
#include <time.h>
#include <tntdb/error.h>
#include <tntdb/result.h>
#include <tntdb/row.h>
#include <tntdb/transaction.h>

#define MAX_CREATE_RETRY 10

namespace DBAssetsInsert {

static const std::string ins_upd_ass_ext_att_QUERY =
    " INSERT INTO"
    "   t_bios_asset_ext_attributes"
    "   (keytag, value, id_asset_element, read_only)"
    " VALUES"
    "  ( :keytag, :value, :element, :readonly)"
    " ON DUPLICATE KEY"
    "   UPDATE"
    "       value = VALUES (value),"
    "       read_only = 1,"
    "       id_asset_ext_attribute = LAST_INSERT_ID(id_asset_ext_attribute)";
// update doesnt return id of updated row -> use workaround

static const std::string ins_ass_ext_att_QUERY =
    " INSERT INTO"
    "   t_bios_asset_ext_attributes"
    "   (keytag, value, id_asset_element, read_only)"
    " SELECT"
    "   :keytag, :value, :element, :readonly"
    " FROM"
    "   t_empty"
    " WHERE NOT EXISTS"
    "   ("
    "       SELECT"
    "           id_asset_element"
    "       FROM"
    "           t_bios_asset_ext_attributes"
    "       WHERE"
    "           keytag = :keytag AND"
    "           id_asset_element = :element"
    "   )";

static db_reply_t insert_into_asset_ext_attribute_template(tntdb::Connection& conn, const char* value,
    const char* keytag, uint32_t asset_element_id, bool read_only, std::string query)
{
    LOG_START;

    uint32_t newid = 0;
    uint32_t n     = 0; // number of rows affected

    db_reply_t ret = db_reply_new();
    // input parameters control
    if (asset_element_id == 0) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = "appropriate asset element is not specified";
        log_error(
            "end: ignore insert, appropriate asset element is "
            "not specified");
        return ret;
    }
    if (!persist::is_ok_value(value)) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = "unexpected value"; /* Not from list of correct values */
        log_error("end: ignore insert, unexpected value '%s'", value);
        return ret;
    }
    if (!persist::is_ok_keytag(keytag)) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = "unacceptable keytag"; /* Some key names are forbidden */
        log_error("end: ignore insert, unacceptable keytag '%s'", keytag);
        return ret;
    }

    try {

        tntdb::Statement st = conn.prepareCached(query);

        n = st.set("keytag", keytag)
                .set("value", value)
                .set("readonly", read_only)
                .set("element", asset_element_id)
                .execute();
        newid = uint32_t(conn.lastInsertId());
        log_debug("was inserted %" PRIu32 " rows", n);
        ret.affected_rows = n;
        ret.rowid         = newid;
        // attention:
        //  -- 0 rows can be inserted
        //        - there is no free space
        //        - FK on id_asset_element
        //        - row is already inserted
        //        - in some other, but not normal cases
        //  -- 1 row is inserted - a usual case
        //  -- more than one row, it is not normal and it is not expected
        //       due to nature of the insert statement
    } catch (const std::exception& e) {
        ret.affected_rows = n;
        ret.status        = 0;
        ret.errtype       = DB_ERR;
        ret.errsubtype    = DB_ERROR_INTERNAL;
        ret.msg           = e.what();
        LOG_END_ABNORMAL(e);
        return ret;
    }
    // a statement "insert on duplicate update
    // return 2 affected rows when update is used and updated value was different from previos
    // return 0 affected rows when update is used and updated value is the same as previos
    if ((n == 1) || (((n == 2) || (n == 0)) && (read_only))) {
        ret.status = 1;
        LOG_END;
    } else {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = "unexpected number of returned rows";
        log_info("end: %" PRIu32 " - unexpected number of rows returned", n);
    }
    return ret;
}

db_reply_t insert_into_asset_ext_attribute(
    tntdb::Connection& conn, const char* value, const char* keytag, uint32_t asset_element_id, bool read_only)
{
    if (!read_only) {
        log_debug("use pure insert");
        return insert_into_asset_ext_attribute_template(
            conn, value, keytag, asset_element_id, read_only, ins_ass_ext_att_QUERY);
    } else {
        log_debug("use insert on duplicate update");
        return insert_into_asset_ext_attribute_template(
            conn, value, keytag, asset_element_id, read_only, ins_upd_ass_ext_att_QUERY);
    }
}

// sql_plac: generate the placeholder name
// example: sql_plac(2, 3) -> "item2_3";
static std::string sql_plac(size_t i, size_t j)
{
    return "item" + std::to_string(i) + "_" + std::to_string(j);
}

// multi_insert_string: generate the SQL string for multivalue insert
// Example:
// multi_insert_string("INSERT INTO t_bios_foo", 2, 3, "ON DUPLICATE KEY ....") ->
// 'INSERT INTO t_bios_foo (foo, bar)
// VALUES(:item0_0, :item0_1),
// (:item1_0, :item1_1),
// (:item2_0, :item2_1)
//  ON DUPLICATE KEY UPDATE ...'
static std::string multi_insert_string(
    const std::string& sql_header, size_t tuple_len, size_t items_len, const std::string& sql_postfix)
{
    std::stringstream s{};

    s << sql_header;
    s << "\nVALUES ";
    for (size_t i = 0; i != items_len; i++) {
        s << "(";
        for (size_t j = 0; j != tuple_len; j++) {
            s << ":" << sql_plac(i, j);
            if (j < tuple_len - 1)
                s << ", ";
        }
        if (i < items_len - 1)
            s << "),\n";
        else
            s << ")\n";
    }
    s << sql_postfix;
    return s.str();
}

// generate the proper tntdb::Statement for multi value insert for extended attributes
static tntdb::Statement s_multi_insert_statement(
    tntdb::Connection& conn, uint32_t element_id, bool read_only, zhash_t* attributes)
{
    static const std::string sql_header =
        "INSERT INTO "
        "   t_bios_asset_ext_attributes (keytag, value, id_asset_element, read_only) ";
    static const std::string sql_postfix =
        " ON DUPLICATE KEY "
        "   UPDATE "
        "       id_asset_ext_attribute = LAST_INSERT_ID(id_asset_ext_attribute) ";
    auto sql = multi_insert_string(sql_header, 4, zhash_size(attributes), sql_postfix);

    log_debug("sql: '%s'", sql.c_str());
    auto st = conn.prepare(sql);

    char*  value = static_cast<char*>(zhash_first(attributes)); // first value
    size_t i     = 0;
    while (value != nullptr) {
        char* key = const_cast<char*>(zhash_cursor(attributes)); // key of this value
        st.set(sql_plac(i, 0), key);
        st.set(sql_plac(i, 1), value);
        st.set(sql_plac(i, 2), element_id);
        st.set(sql_plac(i, 3), read_only);
        value = static_cast<char*>(zhash_next(attributes)); // next value
        i++;
    }

    return st;
}

db_reply_t insert_into_asset_ext_attributes(
    tntdb::Connection& conn, uint32_t element_id, zhash_t* attributes, bool read_only, std::string& /*err*/)
{
    LOG_START;
    size_t i = 0;

    db_reply_t ret = db_reply_new();
    if (!attributes) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = TRANSLATE_ME("no attributes to insert (NULL)");
        log_error("end: %s, %s", "ignore insert", ret.msg.c_str());
        return ret;
    }
    if (zhash_size(attributes) == 0) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = TRANSLATE_ME("no attributes to insert (size == 0)");
        log_error("end: %s, %s", "ignore insert", ret.msg.c_str());
        return ret;
    }

    try {
        auto st = s_multi_insert_statement(conn, element_id, read_only, attributes);
        i       = st.execute();
        log_debug("%zu attributes written", i);
        ret.status = 1;
        LOG_END;
        return ret;
    } catch (const std::exception& e) {
        ret.affected_rows = i;
        ret.status        = 0;
        ret.errtype       = DB_ERR;
        ret.errsubtype    = DB_ERROR_INTERNAL;
        ret.msg           = e.what();
        LOG_END_ABNORMAL(e);
        return ret;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////

static std::string grp_values(std::set<uint32_t> const& groups, uint32_t asset_element_id)
{
    std::string result = "  ";
    for (auto& grp : groups) {
        result = result + "(" + std::to_string(grp) + "," + std::to_string(asset_element_id) + ")" + ",";
    }
    // need to remove last ","
    result.back() = ' ';
    return result;
}

db_reply_t insert_asset_element_into_asset_group(tntdb::Connection& conn, uint32_t group_id, uint32_t asset_element_id)
{
    LOG_START;
    log_debug("  group_id = %" PRIu32, group_id);
    log_debug("  asset_element_id = %" PRIu32, asset_element_id);

    db_reply_t ret = db_reply_new();

    // input parameters control
    if (asset_element_id == 0) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = "0 value of asset_element_id is not allowed";
        log_error("end: %s, %s", "ignore insert", ret.msg.c_str());
        return ret;
    }
    if (group_id == 0) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = "0 value of group_id is not allowed";
        log_error("end: %s, %s", "ignore insert", ret.msg.c_str());
        return ret;
    }

    try {
        tntdb::Statement st = conn.prepareCached(
            " INSERT INTO"
            "   t_bios_asset_group_relation"
            "   (id_asset_group, id_asset_element)"
            " SELECT"
            "   :group, :element"
            " FROM"
            "   t_empty"
            " WHERE NOT EXISTS"
            "   ("
            "       SELECT"
            "           id_asset_group"
            "       FROM"
            "           t_bios_asset_group_relation"
            "       WHERE"
            "           id_asset_group = :group AND"
            "           id_asset_element = :element"
            "   )");

        ret.affected_rows = st.set("group", group_id).set("element", asset_element_id).execute();
        ret.rowid         = uint32_t(conn.lastInsertId());
        log_debug("[t_bios_asset_group_relation]: was inserted %" PRIu64 " rows", ret.affected_rows);
        ret.status = 1;
        LOG_END;
        return ret;
    } catch (const std::exception& e) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_INTERNAL;
        ret.msg        = e.what();
        LOG_END_ABNORMAL(e);
        return ret;
    }
}

db_reply_t insert_element_into_groups(
    tntdb::Connection& conn, std::set<uint32_t> const& groups, uint32_t asset_element_id)
{
    LOG_START;
    log_debug("  asset_element_id = %" PRIu32, asset_element_id);

    db_reply_t ret = db_reply_new();

    // input parameters control
    if (asset_element_id == 0) {
        ret.status  = 0;
        ret.errtype = REQUEST_PARAM_BAD_ERR;
        log_error("end: %s, %s", "ignore insert", "0 value of asset_element_id is not allowed");
        return ret;
    }
    if (groups.empty()) {
        log_debug("nothing to insert");
        ret.status = 1;
        LOG_END;
        // actually, if there is nothing to insert, then insert was ok :)
        return ret;
    }
    log_debug("input parameters are correct");

    try {
        tntdb::Statement st = conn.prepare(
            " INSERT INTO"
            "   t_bios_asset_group_relation"
            "   (id_asset_group, id_asset_element)"
            " VALUES " +
            grp_values(groups, asset_element_id));

        ret.affected_rows = st.execute();
        log_debug("[t_bios_asset_group_relation]: was inserted %" PRIu64 " rows", ret.affected_rows);

        if (ret.affected_rows == groups.size()) {
            ret.status = 1;
            log_debug("all links were inserted successfully");
            LOG_END;
        } else {
            ret.status  = 0;
            ret.errtype = INTERNAL_ERR;
            ret.msg     = TRANSLATE_ME("not all links were inserted");
            log_error("end: %s", ret.msg.c_str());
        }
        return ret;
    } catch (const std::exception& e) {
        LOG_END_ABNORMAL(e);
        ret.status  = 0;
        ret.errtype = INTERNAL_ERR;
        return ret;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: check, if it works with multiple powerlinks between two devices
db_reply_t insert_into_asset_link(tntdb::Connection& conn, uint32_t asset_element_src_id,
    uint32_t asset_element_dest_id, uint8_t link_type_id, const char* src_out, const char* dest_in)
{
    LOG_START;

    db_reply_t ret = db_reply_new();

    // input parameters control
    if (asset_element_dest_id == 0) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = "destination device is not specified";
        log_error("end: %s, %s", "ignore insert", ret.msg.c_str());
        return ret;
    }
    if (asset_element_src_id == 0) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = "source device is not specified";
        log_error("end: %s, %s", "ignore insert", ret.msg.c_str());
        return ret;
    }
    if (!persist::is_ok_link_type(link_type_id)) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = "wrong link type";
        log_error("end: %s, %s", "ignore insert", ret.msg.c_str());
        return ret;
    }
    // src_out and dest_in can take any value from available range
    log_debug("input parameters are correct");

    try {
        tntdb::Statement st = conn.prepareCached(
            " INSERT INTO"
            "   t_bios_asset_link"
            "   (id_asset_device_src, id_asset_device_dest,"
            "        id_asset_link_type, src_out, dest_in)"
            " SELECT"
            "   v1.id_asset_element, v2.id_asset_element, :linktype,"
            "   :out, :in"
            " FROM"
            "   v_bios_asset_device v1," // src
            "   v_bios_asset_device v2"  // dvc
            " WHERE"
            "   v1.id_asset_element = :src AND"
            "   v2.id_asset_element = :dest AND"
            "   NOT EXISTS"
            "     ("
            "           SELECT"
            "             id_link"
            "           FROM"
            "             t_bios_asset_link v3"
            "           WHERE"
            "               v3.id_asset_device_src = v1.id_asset_element AND"
            "               v3.id_asset_device_dest = v2.id_asset_element AND"
            "               ( ((v3.src_out = :out) AND (v3.dest_in = :in)) ) AND"
            "               v3.id_asset_device_dest = v2.id_asset_element"
            "    )");

        if (!src_out || strcmp(src_out, "") == 0)
            st = st.setNull("out");
        else
            st = st.set("out", src_out);

        if (!dest_in || strcmp(dest_in, "") == 0)
            st = st.setNull("in");
        else
            st = st.set("in", dest_in);

        ret.affected_rows = st.set("src", asset_element_src_id)
                                .set("dest", asset_element_dest_id)
                                .set("linktype", link_type_id)
                                .execute();

        ret.rowid = uint32_t(conn.lastInsertId());
        log_debug("[t_bios_asset_link]: was inserted %" PRIu64 " rows", ret.affected_rows);
        ret.status = 1;
        LOG_END;
        return ret;
    } catch (const std::exception& e) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_INTERNAL;
        ret.msg        = e.what();
        LOG_END_ABNORMAL(e);
        return ret;
    }
}

db_reply_t insert_into_new_asset_links(tntdb::Connection& conn, std::vector<new_link_t> const& links)
{
    std::vector<link_t> oldLinks;

    for (const auto& l : links) {
        link_t oldLink;

        oldLink.src     = uint32_t(DBAssets::name_to_asset_id(l.src));
        oldLink.dest    = uint32_t(DBAssets::name_to_asset_id(l.dest));
        oldLink.src_out = l.src_out;
        oldLink.dest_in = l.dest_in;
        oldLink.type    = l.type;

        oldLinks.emplace_back(oldLink);
    }

    return insert_into_asset_links(conn, oldLinks);
}

db_reply_t insert_into_asset_links(tntdb::Connection& conn, std::vector<link_t> const& links)
{
    LOG_START;

    db_reply_t ret = db_reply_new();

    // input parameters control
    if (links.empty()) {
        log_debug("nothing to insert");
        ret.status = 1;
        LOG_END;
        // actually, if there is nothing to insert, then insert was ok :)
        return ret;
    }
    log_debug("input parameters are correct");

    for (auto& one_link : links) {
        auto ret_internal = DBAssetsInsert::insert_into_asset_link(
            conn, one_link.src, one_link.dest, uint8_t(one_link.type), one_link.src_out, one_link.dest_in);
        if (ret_internal.status == 1)
            ret.affected_rows++;
    }
    if (ret.affected_rows == links.size()) {
        ret.status = 1;
        log_debug("all links were inserted successfully");
        LOG_END;
        return ret;
    } else {
        ret.status  = 0;
        ret.errtype = INTERNAL_ERR;
        log_error("end: not all links were inserted");
        LOG_END;
        return ret;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

db_reply_t insert_into_asset_element(tntdb::Connection& conn, const char* element_name, uint16_t element_type_id,
    uint32_t parent_id, const char* status, uint16_t priority, uint16_t subtype_id, const char* asset_tag, bool update)
{
    LOG_START;
    log_debug("  element_name = '%s'", element_name);
    if (subtype_id == 0)
        subtype_id = persist::asset_subtype::N_A;

    db_reply_t ret = db_reply_new();

    // input parameters control
    if (!persist::is_ok_name(element_name)) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        //        bios_error_idx (ret.rowid, ret.msg, "request-param-bad", "element_type_id", element_type_id, "<valid
        //        element type id>");
        log_error("end: %s, %s", "ignore insert", ret.msg.c_str());
        return ret;
    }

    if (!persist::is_ok_element_type(element_type_id)) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = TRANSLATE_ME("0 value of element_type_id is not allowed");
        log_error("end: %s, %s", "ignore insert", ret.msg.c_str());
        return ret;
    }

    // ASSUMPTION: all datacenters are unlocated elements
    if (element_type_id == persist::asset_type::DATACENTER && parent_id != 0) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        return ret;
    }
    log_debug("input parameters are correct");

    try {
        // this concat with last_insert_id may have raise condition issue but hopefully is not important
        tntdb::Statement statement;
        if (update) {
            statement = conn.prepareCached(
                " INSERT INTO t_bios_asset_element "
                " (name, id_type, id_subtype, id_parent, status, priority, asset_tag) "
                " VALUES "
                " (:name, :id_type, :id_subtype, :id_parent, :status, :priority, :asset_tag) "
                " ON DUPLICATE KEY UPDATE name = :name ");
        } else {
            timeval t;

            bool        valid = false;
            std::string indexStr;

            unsigned retry = 0;

            while (!valid && (retry++ < MAX_CREATE_RETRY)) {
                gettimeofday(&t, nullptr);
                srand(static_cast<unsigned int>(t.tv_sec * t.tv_usec));
                // generate 8 digit random integer
                unsigned long index = static_cast<unsigned long>(rand()) % static_cast<unsigned long>(100000000);

                indexStr = std::to_string(index);
                // create 8 digit index with leading zeros
                indexStr = std::string(8 - indexStr.length(), '0') + indexStr;

                log_debug("Checking ID %s validity", indexStr.c_str());

                auto q = conn.prepare(R"(
                    SELECT COUNT(id_asset_element) as cnt
                    FROM t_bios_asset_element
                    WHERE name like :name
                )");

                q.set("name", "%" + indexStr);

                try {
                    int res = q.selectValue().getInt();
                    valid   = (res == 0);
                } catch (const std::exception& e) {
                    throw std::runtime_error(e.what());
                }
            }

            if (!valid) {
                throw std::runtime_error("Multiple Asset ID collisions - impossible to create asset");
            }

            statement = conn.prepareCached(
                " INSERT INTO t_bios_asset_element "
                " (name, id_type, id_subtype, id_parent, status, priority, asset_tag) "
                " VALUES "
                " (concat (:name, '-', \"" +
                indexStr + "\"), :id_type, :id_subtype, :id_parent, :status, :priority, :asset_tag) ");
        }
        if (parent_id == 0) {
            ret.affected_rows = statement.set("name", element_name)
                                    .set("id_type", element_type_id)
                                    .set("id_subtype", subtype_id)
                                    .setNull("id_parent")
                                    .set("status", status)
                                    .set("priority", priority)
                                    .set("asset_tag", asset_tag)
                                    .execute();
        } else {
            ret.affected_rows = statement.set("name", element_name)
                                    .set("id_type", element_type_id)
                                    .set("id_subtype", subtype_id)
                                    .set("id_parent", parent_id)
                                    .set("status", status)
                                    .set("priority", priority)
                                    .set("asset_tag", asset_tag)
                                    .execute();
        }

        ret.rowid = uint32_t(conn.lastInsertId());
        log_debug("[t_bios_asset_element]: was inserted %" PRIu64 " rows", ret.affected_rows);

        if (ret.affected_rows == 0) {
            ret.status     = 0;
            ret.errtype    = DB_ERR;
            ret.errsubtype = DB_ERROR_BADINPUT;
        } else
            ret.status = 1;
        LOG_END;
        return ret;
    } catch (const std::exception& e) {
        LOG_END_ABNORMAL(e);
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_INTERNAL;
        return ret;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: inserted data are probably unused, check and remove
db_reply_t insert_into_monitor_asset_relation(tntdb::Connection& conn, uint16_t monitor_id, uint32_t element_id)
{
    LOG_START;
    log_debug("  monitor_id = %" PRIu32, monitor_id);
    log_debug("  element_id = %" PRIu32, element_id);

    db_reply_t ret = db_reply_new();

    // input parameters control
    if (element_id == 0) {
        ret.status  = 0;
        ret.errtype = INTERNAL_ERR;
        log_error("end: %s, %s", "ignore insert", "0 value of element_id is not allowed");
        return ret;
    }
    if (monitor_id == 0) {
        ret.status  = 0;
        ret.errtype = INTERNAL_ERR;
        ret.msg     = TRANSLATE_ME("0 value of monitor_id is not allowed");
        log_error("end: %s, %s", "ignore insert", "0 value of monitor_id is not allowed");
        return ret;
    }
    log_debug("input parameters are correct");

    try {
        tntdb::Statement st = conn.prepareCached(
            " INSERT INTO"
            "   t_bios_monitor_asset_relation"
            "   (id_discovered_device, id_asset_element)"
            " VALUES"
            "   (:monitor, :asset)");

        ret.affected_rows = st.set("monitor", monitor_id).set("asset", element_id).execute();
        ret.rowid         = uint32_t(conn.lastInsertId());
        log_debug("[t_bios_monitor_asset_relation]: was inserted %" PRIu64 " rows", ret.affected_rows);
        ret.status = 1;
        LOG_END;
        return ret;
    } catch (const std::exception& e) {
        LOG_END_ABNORMAL(e);
        ret.status  = 0;
        ret.errtype = INTERNAL_ERR;
        return ret;
    }
}

// TODO: inserted data are probably unused, check and remove
db_reply_t insert_into_monitor_device(tntdb::Connection& conn, uint16_t device_type_id, const char* device_name)
{
    LOG_START;

    db_reply_t ret = db_reply_new();
    try {
        tntdb::Statement st = conn.prepareCached(
            " INSERT INTO"
            "   t_bios_discovered_device (name, id_device_type)"
            " VALUES (:name, :iddevicetype)"
            " ON DUPLICATE KEY"
            "   UPDATE"
            "       id_discovered_device = LAST_INSERT_ID(id_discovered_device)");

        // Insert one row or nothing
        ret.affected_rows = st.set("name", device_name).set("iddevicetype", device_type_id).execute();
        log_debug("[t_bios_discovered_device]: was inserted %" PRIu64 " rows", ret.affected_rows);
        ret.rowid  = uint32_t(conn.lastInsertId());
        ret.status = 1;
        LOG_END;
        return ret;
    } catch (const std::exception& e) {
        LOG_END_ABNORMAL(e);
        ret.status  = 0;
        ret.errtype = INTERNAL_ERR;
        return ret;
    }
}
} // namespace DBAssetsInsert
