/*  =========================================================================
    fty_common_db_asset_delete - Functions deleting assets from database.

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
    fty_common_db_asset_delete - Functions deleting assets from database.
@discuss
@end
*/

#include "fty_common_db.h"
#include <fty_common_asset_types.h>
#include <fty_log.h>

namespace DBAssetsDelete {

// ATTENTION: in theory there could exist more than one link
// between two devices
// FIXME: unused function except in fty-rest tests
db_reply_t delete_asset_link(tntdb::Connection& conn, uint32_t asset_element_id_src, uint32_t asset_element_id_dest)
{
    LOG_START;
    log_debug("  asset_element_id_src = %" PRIu32, asset_element_id_src);
    log_debug("  asset_element_id_dest = %" PRIu32, asset_element_id_dest);

    db_reply_t ret = db_reply_new();

    // input parameters control
    if (asset_element_id_src <= 0) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = "value <= 0 of asset_element_id_src is not allowed";
        log_error("end: %s, %s", "ignore delete", ret.msg.c_str());
        return ret;
    }
    if (asset_element_id_dest <= 0) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = "value <= 0 of asset_element_id_dest is not allowed";
        log_error("end: %s, %s", "ignore delete", ret.msg.c_str());
        return ret;
    }
    log_debug("input parameters are correct");

    try {
        tntdb::Statement st = conn.prepareCached(
            " DELETE"
            " FROM"
            "   t_bios_asset_link"
            " WHERE"
            "   id_asset_device_src = :src AND"
            "   id_asset_device_dest = :dest");

        ret.affected_rows = st.set("src", asset_element_id_src).set("dest", asset_element_id_dest).execute();
        log_debug("[t_bios_asset_link]: was deleted %" PRIu64 " rows", ret.affected_rows);
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

db_reply_t delete_asset_links_to(tntdb::Connection& conn, uint32_t asset_device_id)
{
    LOG_START;
    log_debug("  asset_device_id = %" PRIu32, asset_device_id);

    db_reply_t ret = db_reply_new();

    try {
        tntdb::Statement st = conn.prepareCached(
            " DELETE FROM"
            "   t_bios_asset_link"
            " WHERE"
            "   id_asset_device_dest = :dest");

        ret.affected_rows = st.set("dest", asset_device_id).execute();
        log_debug("[t_bios_asset_link]: was deleted %" PRIu64 " rows", ret.affected_rows);
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

////////////////////////////////////////////////////////////////////////////////////////////

db_reply_t delete_asset_group_links(tntdb::Connection& conn, uint32_t asset_group_id)
{
    LOG_START;
    log_debug("  asset_group_id = %" PRIu32, asset_group_id);

    db_reply_t ret = db_reply_new();

    try {
        tntdb::Statement st = conn.prepareCached(
            " DELETE FROM"
            "   t_bios_asset_group_relation"
            " WHERE"
            "   id_asset_group = :grp");

        ret.affected_rows = st.set("grp", asset_group_id).execute();
        log_debug("[t_bios_asset_group_relation]: was deleted %" PRIu64 " rows", ret.affected_rows);
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

//////////////////////////////////////////////////////////////////////////////////////

// FIXME: unused function except in fty-rest tests
db_reply_t delete_asset_ext_attribute(tntdb::Connection& conn, const char* keytag, uint32_t asset_element_id)
{
    LOG_START;

    db_reply_t ret = db_reply_new();

    // input parameters control
    if (!persist::is_ok_keytag(keytag)) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = "unexpected value of keytag";
        log_error("end: %s, %s", "ignore delete", ret.msg.c_str());
        return ret;
    }
    if (asset_element_id <= 0) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_BADINPUT;
        ret.msg        = "value <= 0 of asset_element_id is not allowed";
        log_error("end: %s, %s", "ignore delete", ret.msg.c_str());
        return ret;
    }
    log_debug("input parameters are correct");

    try {
        tntdb::Statement st = conn.prepareCached(
            " DELETE FROM"
            "   t_bios_asset_ext_attributes"
            " WHERE"
            "   keytag = :keytag AND"
            "   id_asset_element = :element");

        ret.affected_rows = st.set("keytag", keytag).set("element", asset_element_id).execute();
        log_debug("[t_bios_asset_ext_attributes]: was deleted %" PRIu64 " rows", ret.affected_rows);
        if ((ret.affected_rows == 1) || (ret.affected_rows == 0)) {
            ret.status = 1;
            LOG_END;
            return ret;
        } else {
            ret.status     = 0;
            ret.errtype    = DB_ERR;
            ret.errsubtype = DB_ERROR_DELETEFAIL;
            ret.msg        = "unexpected number of rows was deleted";
            log_error("end: %s", ret.msg.c_str());
            return ret;
        }
    } catch (const std::exception& e) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_INTERNAL;
        ret.msg        = e.what();
        LOG_END_ABNORMAL(e);
        return ret;
    }
}

db_reply_t delete_asset_ext_attributes_with_ro(tntdb::Connection& conn, uint32_t asset_element_id, bool read_only)
{
    LOG_START;
    log_debug("read_only = %i, asset_element_id = %" PRIu32, read_only, asset_element_id);

    db_reply_t ret = db_reply_new();

    try {
        tntdb::Statement st = conn.prepareCached(
            " DELETE FROM"
            "   t_bios_asset_ext_attributes"
            " WHERE"
            "   id_asset_element = :element AND "
            "   read_only = :ro ");

        ret.affected_rows = st.set("element", asset_element_id).set("ro", read_only).execute();
        log_debug("[t_bios_asset_ext_attributes]: was deleted %" PRIu64 " rows", ret.affected_rows);
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

db_reply_t delete_asset_element(tntdb::Connection& conn, uint32_t asset_element_id)
{
    LOG_START;
    log_debug("asset_element_id = %" PRIu32, asset_element_id);

    db_reply_t ret = db_reply_new();

    try {
        tntdb::Statement st = conn.prepareCached(
            " DELETE FROM"
            "   t_bios_asset_element"
            " WHERE"
            "   id_asset_element = :element");

        ret.affected_rows = st.set("element", asset_element_id).execute();
        log_debug("[t_bios_asset_element]: was deleted %" PRIu64 " rows", ret.affected_rows);
        if ((ret.affected_rows == 1) || (ret.affected_rows == 0)) {
            ret.status = 1;
            LOG_END;
            return ret;
        } else {
            ret.status     = 0;
            ret.errtype    = DB_ERR;
            ret.errsubtype = DB_ERROR_DELETEFAIL;
            ret.msg        = "unexpected number of rows was deleted";
            log_error("end: %s", ret.msg.c_str());
            return ret;
        }
    } catch (const std::exception& e) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_INTERNAL;
        ret.msg        = e.what();
        LOG_END_ABNORMAL(e);
        return ret;
    }
}

db_reply_t delete_asset_element_from_asset_groups(tntdb::Connection& conn, uint32_t asset_element_id)
{
    LOG_START;
    log_debug("asset_element_id = %" PRIu32, asset_element_id);

    db_reply_t ret = db_reply_new();

    try {
        tntdb::Statement st = conn.prepareCached(
            " DELETE FROM"
            "   t_bios_asset_group_relation"
            " WHERE"
            "   id_asset_element = :element");

        ret.affected_rows = st.set("element", asset_element_id).execute();
        log_debug("[t_bios_asset_group_relation]: was deleted %" PRIu64 " rows", ret.affected_rows);
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

db_reply_t delete_asset_element_from_asset_group(
    tntdb::Connection& conn, uint32_t asset_group_id, uint32_t asset_element_id)
{
    LOG_START;
    log_debug("  asset_group_id = %" PRIu32, asset_group_id);
    log_debug("  asset_element_id = %" PRIu32, asset_element_id);

    db_reply_t ret = db_reply_new();

    try {
        tntdb::Statement st = conn.prepareCached(
            " DELETE FROM"
            "   t_bios_asset_group_relation"
            " WHERE"
            "   id_asset_group = :grp AND"
            "   id_asset_element = :element");

        ret.affected_rows = st.set("grp", asset_group_id).set("element", asset_element_id).execute();
        log_debug("[t_bios_asset_group_relation]: was deleted %" PRIu64 " rows", ret.affected_rows);
        if ((ret.affected_rows == 1) || (ret.affected_rows == 0)) {
            ret.status = 1;
            LOG_END;
            return ret;
        } else {
            ret.status     = 0;
            ret.errtype    = DB_ERR;
            ret.errsubtype = DB_ERROR_DELETEFAIL;
            ret.msg        = "unexpected number of rows was deleted";
            log_error("end: %s", ret.msg.c_str());
            return ret;
        }
    } catch (const std::exception& e) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_INTERNAL;
        ret.msg        = e.what();
        LOG_END_ABNORMAL(e);
        return ret;
    }
}

// TODO: inserted data are probably unused, check and remove
db_reply_t delete_monitor_asset_relation_by_a(tntdb::Connection& conn, uint32_t id)
{
    LOG_START;
    log_debug("  id = %" PRIu32, id);

    db_reply_t ret = db_reply_new();

    try {
        tntdb::Statement st = conn.prepareCached(
            " DELETE FROM"
            "   t_bios_monitor_asset_relation"
            " WHERE"
            "   id_asset_element = :id");

        ret.affected_rows = st.set("id", id).execute();
        log_debug("[t_bios_monitor_asset_relation]: was deleted %" PRIu64 " rows", ret.affected_rows);
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
} // namespace DBAssetsDelete
