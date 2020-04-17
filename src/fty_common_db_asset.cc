/*  =========================================================================
    fty_common_db_asset - Asset related queries

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

/*
@header
    fty_common_db_asset - Asset filtering functions
@discuss
@end
*/

#include "fty_common_db_classes.h"
#include <fty_common_macros.h>
#include <assert.h>

namespace DBAssets {

std::pair <std::string, std::string>
id_to_name_ext_name (uint32_t asset_id)
{
    std::string name;
    std::string ext_name;
    try
    {
        tntdb::Connection conn = tntdb::connect(DBConn::url);
        tntdb::Statement st = conn.prepare(
            " SELECT asset.name, ext.value "
            " FROM "
            "   t_bios_asset_element AS asset "
            " LEFT JOIN "
            "   t_bios_asset_ext_attributes AS ext "
            " ON "
            "   ext.id_asset_element = asset.id_asset_element "
            " WHERE "
            "   ext.keytag = \"name\" AND asset.id_asset_element = :asset_id ");

        tntdb::Row row = st.set("asset_id", asset_id).selectRow();
        log_debug("[t_bios_asset_element]: were selected %" PRIu32 " rows", 1);

        row [0].get (name);
        row [1].get (ext_name);
    }
    catch (const std::exception &e)
    {
        if (asset_id != 0)
            log_error ("exception caught %s - %" PRIu32, e.what (), asset_id);
        name = "";
        ext_name = "";
    }
    return make_pair (name, ext_name);
}

int64_t
name_to_asset_id (std::string asset_name)
{
    try
    {
        int64_t id = 0;

        tntdb::Connection conn = tntdb::connect(DBConn::url);
        tntdb::Statement st = conn.prepare(
                " SELECT id_asset_element"
                " FROM"
                "   t_bios_asset_element"
                " WHERE name = :asset_name"
                );

        tntdb::Row row = st.set("asset_name", asset_name).selectRow();
        log_debug("[t_bios_asset_element]: were selected %" PRIu32 " rows", 1);

        row [0].get(id);
        return id;
    }
    catch (const tntdb::NotFound &e) {
        log_error ("element %s not found", asset_name.c_str ());
        return -1;
    }
    catch (const std::exception &e)
    {
        log_error ("exception caught %s for element %s", e.what (), asset_name.c_str ());
        return -2;
    }
}

int64_t
name_to_asset_id_check_type (const std::string& asset_name, uint16_t asset_type)
{
    try
    {
        int64_t id = 0;

        tntdb::Connection conn = tntdb::connect(DBConn::url);
        tntdb::Statement st = conn.prepare(
                " SELECT id_asset_element"
                " FROM"
                "   t_bios_asset_element"
                " WHERE name = :asset_name AND id_type = :asset_type"
                );

        tntdb::Row row = st.set("asset_name", asset_name).set("asset_type", asset_type).selectRow();
        log_debug("[t_bios_asset_element]: were selected %" PRIu32 " rows", 1);

        row [0].get(id);
        return id;
    }
    catch (const tntdb::NotFound &e) {
        log_error ("no element %s with expected type", asset_name.c_str ());
        return -1;
    }
    catch (const std::exception &e)
    {
        log_error ("exception caught %s for element %s", e.what (), asset_name.c_str ());
        return -2;
    }
}

int64_t
extname_to_asset_id (std::string asset_ext_name)
{
    try
    {
        int64_t id = 0;

        tntdb::Connection conn = tntdb::connect(DBConn::url);
        tntdb::Statement st = conn.prepare(
                " SELECT a.id_asset_element FROM t_bios_asset_element AS a "
                " INNER JOIN t_bios_asset_ext_attributes AS e "
                " ON a.id_asset_element = e.id_asset_element "
                " WHERE keytag = 'name' and value = :extname "
                );

        tntdb::Row row = st.set("extname", asset_ext_name).selectRow();
        log_debug("[t_bios_asset_element]: were selected %" PRIu32 " rows", 1);

        row [0].get(id);
        return id;
    }
    catch (const tntdb::NotFound &e) {
        log_error ("element %s not found", asset_ext_name.c_str ());
        return -1;
    }
    catch (const std::exception &e)
    {
        log_error ("exception caught %s for element '%s'", e.what (), asset_ext_name.c_str ());
        return -2;
    }
}

int
name_to_extname (std::string asset_name, std::string &ext_name)
{
    try
    {
        tntdb::Connection conn = tntdb::connect(DBConn::url);
        tntdb::Statement st = conn.prepare(
                " SELECT e.value FROM t_bios_asset_ext_attributes AS e  "
                " INNER JOIN t_bios_asset_element AS a "
                " ON a.id_asset_element = e.id_asset_element "
                " WHERE keytag = 'name' AND a.name = :asset_name"
                );

        tntdb::Row row = st.set("asset_name", asset_name).selectRow();
        log_debug("[t_bios_asset_element]: were selected %" PRIu32 " rows", 1);

        row [0].get(ext_name);
        return 0;
    }
    catch (const tntdb::NotFound &e) {
        log_error ("element %s not found", asset_name.c_str ());
        return -1;
    }
    catch (const std::exception &e)
    {
        log_error ("exception caught %s for element '%s'", e.what (), asset_name.c_str ());
        return -2;
    }
}

int
extname_to_asset_name (std::string asset_ext_name, std::string &asset_name)
{
    try
    {
        tntdb::Connection conn = tntdb::connect(DBConn::url);
        tntdb::Statement st = conn.prepare(
                " SELECT a.name FROM t_bios_asset_element AS a "
                " INNER JOIN t_bios_asset_ext_attributes AS e "
                " ON a.id_asset_element = e.id_asset_element "
                " WHERE keytag = 'name' and value = :extname "
                );

        tntdb::Row row = st.set("extname", asset_ext_name).selectRow();
        log_debug("[t_bios_asset_element]: were selected %" PRIu32 " rows", 1);

        row [0].get(asset_name);
        return 0;
    }
    catch (const tntdb::NotFound &e) {
        log_error ("element %s not found", asset_ext_name.c_str ());
        return -1;
    }
    catch (const std::exception &e)
    {
        log_error ("exception caught %s for element '%s'", e.what (), asset_ext_name.c_str ());
        return -2;
    }
}

// --------------------------------------------------------------------------

int
select_asset_element_super_parent (
    tntdb::Connection& conn,
    uint32_t id,
    std::function<void(const tntdb::Row&)>& cb)
{
    LOG_START;

    try{
        tntdb::Statement st = conn.prepare(
            " SELECT "
            "   v.id_asset_element as id, "
            "   v.id_parent1 as id_parent1, "
            "   v.id_parent2 as id_parent2, "
            "   v.id_parent3 as id_parent3, "
            "   v.id_parent4 as id_parent4, "
            "   v.id_parent5 as id_parent5, "
            "   v.id_parent6 as id_parent6, "
            "   v.id_parent7 as id_parent7, "
            "   v.id_parent8 as id_parent8, "
            "   v.id_parent9 as id_parent9, "
            "   v.id_parent10 as id_parent10, "
            "   v.name_parent1 as parent_name1, "
            "   v.name_parent2 as parent_name2, "
            "   v.name_parent3 as parent_name3, "
            "   v.name_parent4 as parent_name4, "
            "   v.name_parent5 as parent_name5, "
            "   v.name_parent6 as parent_name6, "
            "   v.name_parent7 as parent_name7, "
            "   v.name_parent8 as parent_name8, "
            "   v.name_parent9 as parent_name9, "
            "   v.name_parent10 as parent_name10, "
            "   v.id_type_parent1 as id_type_parent1, "
            "   v.id_type_parent2 as id_type_parent2, "
            "   v.id_type_parent3 as id_type_parent3, "
            "   v.id_type_parent4 as id_type_parent4, "
            "   v.id_type_parent5 as id_type_parent5, "
            "   v.id_type_parent6 as id_type_parent6, "
            "   v.id_type_parent7 as id_type_parent7, "
            "   v.id_type_parent8 as id_type_parent8, "
            "   v.id_type_parent9 as id_type_parent9, "
            "   v.id_type_parent10 as id_type_parent10, "
            "   v.id_subtype_parent1 as id_subtype_parent1, "
            "   v.id_subtype_parent2 as id_subtype_parent2, "
            "   v.id_subtype_parent3 as id_subtype_parent3, "
            "   v.id_subtype_parent4 as id_subtype_parent4, "
            "   v.id_subtype_parent5 as id_subtype_parent5, "
            "   v.id_subtype_parent6 as id_subtype_parent6, "
            "   v.id_subtype_parent7 as id_subtype_parent7, "
            "   v.id_subtype_parent8 as id_subtype_parent8, "
            "   v.id_subtype_parent9 as id_subtype_parent9, "
            "   v.id_subtype_parent10 as id_subtype_parent10, "
            "   v.name as name, "
            "   v.type_name as type_name, "
            "   v.id_asset_device_type as device_type, "
            "   v.status as status, "
            "   v.asset_tag as asset_tag, "
            "   v.priority as priority, "
            "   v.id_type as id_type "
            " FROM v_bios_asset_element_super_parent AS v "
            " WHERE "
            "   v.id_asset_element = :id "
            );

        tntdb::Result res = st.set ("id", id).select ();

        for (const auto& r: res) {
            cb(r);
        }
        LOG_END;
        return 0;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
}

int
select_asset_element_all_with_warranty_end(
    tntdb::Connection& conn,
    std::function<void(const tntdb::Row&)>& cb)
{
    LOG_START;

    try{
        tntdb::Statement st = conn.prepare(
            " SELECT "
            "   v.name as name, t.keytag as keytag, t.value as date "
            " FROM v_web_element v "
            " JOIN t_bios_asset_ext_attributes t "
            " ON "
            "   v.id = t.id_asset_element "
            " WHERE "
            "   t.keytag='end_warranty_date' "
            );

        tntdb::Result res = st.select();

        for (const auto& r: res) {
            cb(r);
        }
        LOG_END;
        return 0;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
}


int
select_assets_by_container (tntdb::Connection &conn,
                            uint32_t element_id,
                            std::vector<uint16_t> types,
                            std::vector<uint16_t> subtypes,
                            const std::string &without,
                            const std::string &status,
                            std::function<void(const tntdb::Row&)> cb)
{
    LOG_START;
    log_debug ("container element_id = %" PRIu32, element_id);

    try {
        std::string select =
            " SELECT "
            "   v.name, "
            "   v.id_asset_element as asset_id, "
            "   v.id_asset_device_type as subtype_id, "
            "   v.type_name as subtype_name, "
            "   v.id_type as type_id "
            " FROM "
            "   v_bios_asset_element_super_parent AS v"
            " WHERE "
            "   :containerid in (v.id_parent1, v.id_parent2, v.id_parent3, "
            "                    v.id_parent4, v.id_parent5, v.id_parent6, "
            "                    v.id_parent7, v.id_parent8, v.id_parent9, "
            "                    v.id_parent10)";
        if (!subtypes.empty()) {
            std::string list;
            for( auto &id: subtypes) list += std::to_string(id) + ",";
            select += " AND v.id_asset_device_type in (" + list.substr(0,list.size()-1) + ")";
        }
        if (!types.empty()) {
            std::string list;
            for( auto &id: types) list += std::to_string(id) + ",";
            select += " AND v.id_type in (" + list.substr(0,list.size()-1) + ")";
        }
        if (status != "") {
            select += " AND v.status = \"" + status + "\"";
        }

        std::string end_select = "" ;
        if (without != "") {
            if(without == "location") {
                select += " AND v.id_parent1 is NULL ";
            } else if (without == "powerchain") {
                end_select += " AND NOT EXISTS "
                        " (SELECT id_asset_device_dest "
                        "  FROM t_bios_asset_link_type as l JOIN t_bios_asset_link as a"
                        "  ON a.id_asset_link_type=l.id_asset_link_type "
                        "  WHERE "
                        "     name=\"power chain\" "
                        "     AND v.id_asset_element=a.id_asset_device_dest)";
            } else {
                end_select += " AND NOT EXISTS "
                        " (SELECT a.id_asset_element "
                        "  FROM "
                        "     t_bios_asset_ext_attributes as a "
                        "  WHERE "
                        "     a.keytag=\"" + without + "\""
                        "     AND v.id_asset_element = a.id_asset_element)";
            }
        }

        select += end_select;

        // Can return more than one row.
        tntdb::Statement st = conn.prepare (select);

        tntdb::Result result = st.set("containerid", element_id).
                                  select();
        log_debug("[v_bios_asset_element_super_parent]: were selected %" PRIu32 " rows",
                                                            result.size());
        for ( auto &row: result ) {
            cb(row);
        }
        LOG_END;
        return 0;
    }
    catch (const std::exception& e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
}

// TODO: is this function used anywhere? I can't find it
int
select_assets_by_container (tntdb::Connection &conn,
                            uint32_t element_id,
                            std::function<void(const tntdb::Row&)> cb,
                            std::string status)
{
    log_debug ("container element_id = %" PRIu32, element_id);

    try {
        // Can return more than one row.
        tntdb::Statement st = conn.prepare(
            " SELECT "
            "   v.name, "
            "   v.id_asset_element as asset_id, "
            "   v.id_asset_device_type as subtype_id, "
            "   v.type_name as subtype_name, "
            "   v.id_type as type_id "
            " FROM "
            "   v_bios_asset_element_super_parent v "
            " WHERE "
            "   :containerid in (v.id_parent1, v.id_parent2, v.id_parent3, "
            "                    v.id_parent4, v.id_parent5, v.id_parent6, "
            "                    v.id_parent7, v.id_parent8, v.id_parent9, "
            "                    v.id_parent10)   AND                      "
            "                    v.status = :vstatus                       "
        );

        tntdb::Result result = st.set("containerid", element_id).
                                  set("vstatus", status).
                                  select();
        log_debug ("[v_bios_asset_element_super_parent]: were selected %" PRIu32 " rows",
                                                            result.size());
        for ( auto &row: result ) {
            cb(row);
        }
        return 0;
    }
    catch (const std::exception& e) {
        log_error ("Error: ",e.what());
        return -1;
    }
}

int
select_assets_by_container (tntdb::Connection &conn,
                            uint32_t element_id,
                            std::function<void(const tntdb::Row&)> cb)
{
    return select_assets_by_container(conn, element_id, {}, {},"","", cb);
}


/**
 * select_assets_by_container_filter: creates condition for type/subtype filtering
 * returns query string for types/subtypes we are interested in
 */
static std::string
select_assets_by_container_filter (
    const std::set<std::string> &types_and_subtypes
)
{
    std::string types, subtypes, filter;

    for (const auto &i: types_and_subtypes) {
        uint16_t t = persist::subtype_to_subtypeid (i);
        if (t != persist::asset_subtype::SUNKNOWN) {
            subtypes +=  "," + std::to_string (t);
        } else {
            t = persist::type_to_typeid (i);
            if (t == persist::asset_type::TUNKNOWN) {
                throw std::invalid_argument ("'" + i + "' is not known type or subtype ");
            }
            types += "," + std::to_string (t);
        }
    }
    if (!types.empty ()) types = types.substr(1);
    if (!subtypes.empty ()) subtypes = subtypes.substr(1);
    if (!types.empty () || !subtypes.empty () ) {
        if (!types.empty ()) {
            filter += " id_type in (" + types + ") ";
            if (!subtypes.empty () ) filter += " OR ";
        }
        if (!subtypes.empty ()) {
            filter += " id_asset_device_type in (" + subtypes + ") ";
        }
    }
    log_debug ("filter: '%s'", filter.c_str ());
    return filter;
}

int
select_assets_by_container_name_filter (tntdb::Connection &conn,
                                        const std::string& container_name,
                                        const std::set <std::string>& filter,
                                        std::vector <std::string>& assets)
{
    uint32_t id = 0;

    try {
        if (! container_name.empty ()) {
            // get container asset id
            tntdb::Statement select_id = conn.prepare(
                " SELECT "
                "   v.id "
                " FROM "
                "   v_bios_asset_element v "
                " WHERE "
                "   v.name = :name "
            );

            tntdb::Row row = select_id.set("name", container_name).
                                selectRow();
            row["id"].get(id);
        }
        //Selects assets in a given container
        std::string request =
            " SELECT "
            "   v.name, "
            "   v.id_asset_element as asset_id, "
            "   v.id_asset_device_type as subtype_id, "
            "   v.type_name as subtype_name, "
            "   v.id_type as type_id "
            " FROM "
            "   v_bios_asset_element_super_parent v "
            " WHERE "
            "   (:containerid in (v.id_parent1, v.id_parent2, v.id_parent3, "
            "                     v.id_parent4, v.id_parent5, v.id_parent6, "
            "                     v.id_parent7, v.id_parent8, v.id_parent9, "
            "                     v.id_parent10) OR :containerid = 0 ) ";

        if(!filter.empty())
            request += " AND ( " + select_assets_by_container_filter (filter) +")";
        log_debug("[v_bios_asset_element_super_parent]: %s", request.c_str());

        // Can return more than one row.
        tntdb::Statement select_data = conn.prepare(request);

        tntdb::Result result = select_data.set("containerid", id).
                                  select();
        log_debug("[v_bios_asset_element_super_parent]: were selected %" PRIu32 " rows",
                                                            result.size());
        for ( auto &row: result ) {
            std::string name;
            row["name"].get (name);
            assets.push_back (name);
        }
        return 0;
    }
    catch (const std::exception& e) {
        log_error ("Error: ", e.what());
        return -1;
    }
}

int
select_assets_by_filter (tntdb::Connection &conn,
                         const std::set<std::string> &types_and_subtypes,
                         std::vector <std::string>& assets)
{
    try {
        std::string request =
            " SELECT "
            "   v.name, "
            "   v.id_asset_element as asset_id, "
            "   v.id_asset_device_type as subtype_id, "
            "   v.type_name as subtype_name, "
            "   v.id_type as type_id "
            " FROM "
            "   v_bios_asset_element_super_parent v ";

        if(!types_and_subtypes.empty())
            request += " WHERE " + select_assets_by_container_filter (types_and_subtypes);
        log_debug("[v_bios_asset_element_super_parent]: %s", request.c_str());
        // Can return more than one row.
        tntdb::Statement st = conn.prepare(request);
        tntdb::Result result = st.select();
        log_debug("[v_bios_asset_element_super_parent]: were selected %" PRIu32 " rows",
                                                            result.size());
        for ( auto &row: result ) {
            std::string name;
            row["name"].get (name);
            assets.push_back (name);
        }
        return 0;
    }
    catch (const std::exception& e) {
        log_error ("Error: ",e.what());
        return -1;
    }
}

int
select_assets_without_container (tntdb::Connection &conn,
                                 std::vector<uint16_t> types,
                                 std::vector<uint16_t> subtypes,
                                 std::function<void(const tntdb::Row&)> cb)
{
    LOG_START;

    try {
        std::string select =
            " SELECT "
            "   t.name, "
            "   t.id_asset_element as asset_id, "
            "   t.id_type as type_id, "
            "   t.id_subtype as subtype_id "
            " FROM "
            "   t_bios_asset_element AS t "
            " WHERE "
            "   t.id_parent is NULL";
        if (!subtypes.empty()) {
            std::string list;
            for( auto &id: subtypes) list += std::to_string(id) + ",";
            select += " and t.id_subtype in (" + list.substr(0,list.size()-1) + ")";
        }
        if (!types.empty()) {
            std::string list;
            for( auto &id: types) list += std::to_string(id) + ",";
            select += " and t.id_type in (" + list.substr(0,list.size()-1) + ")";
        }
        // Can return more than one row.
        tntdb::Statement st = conn.prepare (select);

        tntdb::Result result = st.select();
        log_debug ("[t_bios_asset_element]: were selected %" PRIu32 " rows",
                                                            result.size());
        for ( auto &row: result ) {
            cb(row);
        }
        LOG_END;
        return 0;
    }
    catch (const std::exception& e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
}

int
select_assets_all_container (tntdb::Connection &conn,
                             std::vector<uint16_t> types,
                             std::vector<uint16_t> subtypes,
                             const std::string &without,
                             const std::string &status,
                             std::function<void(const tntdb::Row&)> cb)
{
    LOG_START;

    try {
        std::string select =
            " SELECT "
            "   t.name, "
            "   t.id_asset_element as asset_id, "
            "   t.id_type as type_id, "
            "   t.id_subtype as subtype_id "
            " FROM "
            "   t_bios_asset_element as t";

        if(!subtypes.empty() || !types.empty() || status != "" || without != "") {
          select += " WHERE ";
        }
        if (!subtypes.empty()) {
            std::string list;
            for( auto &id: subtypes) list += std::to_string(id) + ",";
            select += " t.id_subtype in (" + list.substr(0,list.size()-1) + ")";
        }
        if (!types.empty()) {
            if(!subtypes.empty() ) {
              select += " AND ";
            }
            std::string list;
            for( auto &id: types) list += std::to_string(id) + ",";
            select += " t.id_type in (" + list.substr(0,list.size()-1) + ")";
        }
        if (status != "") {
            if(!subtypes.empty() || !types.empty()) {
              select += " AND ";
            }
            select += " t.status = \"" + status + "\"";
        }

        std::string end_select = "" ;
        if (without != "") {
            if(!subtypes.empty() || !types.empty() || status != "" ) {
              select += " AND ";
            }
            if(without == "location") {
                select += " t.id_parent is NULL ";
            } else if (without == "powerchain") {
                end_select += " NOT EXISTS "
                        " (SELECT id_asset_device_dest "
                        "  FROM t_bios_asset_link_type as l JOIN t_bios_asset_link as a"
                        "  ON a.id_asset_link_type=l.id_asset_link_type "
                        "  WHERE "
                        "     name=\"power chain\" "
                        "     AND t.id_asset_element=a.id_asset_device_dest)";
            } else {
                end_select += " NOT EXISTS "
                        " (SELECT a.id_asset_element "
                        "  FROM "
                        "     t_bios_asset_ext_attributes as a "
                        "  WHERE "
                        "     a.keytag=\"" + without + "\""
                        "     AND t.id_asset_element = a.id_asset_element)";
            }
        }

        select += end_select;

        // Can return more than one row.
        tntdb::Statement st = conn.prepare (select);

        tntdb::Result result = st.select();
        log_debug("[t_bios_asset_element]: were selected %" PRIu32 " rows",
                                                            result.size());
        for ( auto &row: result ) {
            cb(row);
        }
        LOG_END;
        return 0;
    }
    catch (const std::exception& e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
}

int
select_asset_element_by_dc
    (tntdb::Connection& conn,
     int64_t dc_id,
     std::function<void(const tntdb::Row&)> cb)
{
    LOG_START;

    try{
        // use nested SELECT instead of three-table JOIN
        std::string st =
            "SELECT "
            "   v.id, v.name, v.type_name, "
            "   v.subtype_name, v.id_parent, "
            "   v.status, v.priority, "
            "   v.asset_tag "
            " FROM "
            "   v_web_element v "
            "WHERE "
            "   v.id in "
            "   ( "
                " SELECT p.id_asset_element "
                " FROM v_bios_asset_element_super_parent p "
                " WHERE "
                "   :containerid in ( p.id_asset_element, p.id_parent1, p.id_parent2, "
                "                     p.id_parent3, p.id_parent4, p.id_parent5, "
                "                     p.id_parent6, p.id_parent7, p.id_parent8, "
                "                     p.id_parent9, p.id_parent10) "
            "   ) ";

        //DO NOT CACHE THIS! It will crash MySQL
        tntdb::Statement select_data = conn.prepare(st);

        tntdb::Result result = select_data.set("containerid", dc_id).select();

        for (const auto& r: result) {
            cb(r);
        }
        LOG_END;
        return 0;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
}

int
select_asset_element_all (tntdb::Connection& conn,
                          std::function<void(const tntdb::Row&)>& cb)
{
    LOG_START;

    try{
        tntdb::Statement st = conn.prepare(
            " SELECT"
            "   v.id, v.name, v.type_name,"
            "   v.subtype_name, v.id_parent, v.id_parent_type,"
            "   v.status, v.priority,"
            "   v.asset_tag"
            " FROM"
            "   v_web_element v"
        );

        tntdb::Result res = st.select();

        for (const auto& r: res) {
            cb(r);
        }
        LOG_END;
        return 0;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
}

// TODO: unused, refactor and delete
int
convert_asset_to_monitor (tntdb::Connection &conn,
                          uint32_t asset_element_id,
                          uint16_t &monitor_element_id)
{
    try{
        tntdb::Statement st = conn.prepare(
            " SELECT "
            "   v.id_discovered_device "
            " FROM "
            "   v_bios_monitor_asset_relation v "
            " WHERE "
            "   v.id_asset_element = :id "
        );

        tntdb::Value value = st.set("id", asset_element_id).
                                selectValue();

        value.get(monitor_element_id);
        LOG_END;
        return 0;
    }
    catch (const tntdb::NotFound &e){
        log_info("end: counterpart for %" PRIu32 " notfound", asset_element_id);
        monitor_element_id = 0;
        return 0;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
}

int
count_keytag(
        tntdb::Connection& conn,
        const std::string &keytag,
        const std::string &value)
{
    LOG_START;
    try{
        tntdb::Statement st = conn.prepare(
            " SELECT COUNT( * ) "
            " FROM t_bios_asset_ext_attributes "
            " WHERE keytag = :keytag AND"
            "       value = :value"
        );

        tntdb::Row row = st.set("keytag", keytag)
                           .set("value", value)
                           .selectRow();

        int r = 0;
        row[0].get(r);
        LOG_END;
        return r;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
}

int
unique_keytag (tntdb::Connection &conn,
               const std::string &keytag,
               const std::string &value,
               uint32_t       element_id)
{
    LOG_START;

    try{
        tntdb::Statement st = conn.prepare(
            " SELECT "
            "   id_asset_element "
            " FROM "
            "   t_bios_asset_ext_attributes "
            " WHERE keytag = :keytag AND"
            "       value = :value"
        );

        tntdb::Row row = st.set("keytag", keytag)
                           .set("value", value)
                           .selectRow();

        uint32_t r = 0;
        row[0].get(r);

        LOG_END;
        if ( element_id == r )
            return 0; // is ok
        else
            return 1; // is not ok
    }
    catch (const tntdb::NotFound &e) {
        LOG_END_ABNORMAL(e);
        return 0; // ok
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
}

int
max_number_of_power_links (tntdb::Connection& conn)
{
    LOG_START;

    try{
        tntdb::Statement st = conn.prepare(
            " SELECT "
            "   MAX(power_src_count) "
            " FROM "
            "   ( SELECT COUNT(*) power_src_count FROM t_bios_asset_link "
            "            GROUP BY id_asset_device_dest) pwr "
        );

        tntdb::Row row = st.selectRow();

        int r = 0;
        row[0].get(r);
        LOG_END;
        return r;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
}

int
count_of_link_src (tntdb::Connection& conn,
                   uint32_t id)
{
    LOG_START;
    static const int id_asset_link_type = 1;
    try{
        tntdb::Statement st = conn.prepare(
            " SELECT COUNT( * ) "
            " FROM v_bios_asset_link "
            " WHERE id_asset_element_src = :id AND"
            "       id_asset_link_type = :lt "
        );

        tntdb::Row row = st.\
            set("id", id).\
            set("lt", id_asset_link_type).\
            selectRow();

        int r = 0;
        row[0].get(r);
        LOG_END;
        return r;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
}


int
max_number_of_asset_groups (tntdb::Connection& conn)
{
    LOG_START;

    try{
        tntdb::Statement st = conn.prepare(
            " SELECT "
            "   MAX(grp_count) "
            " FROM "
            "   ( SELECT COUNT(*) grp_count FROM t_bios_asset_group_relation "
            "            GROUP BY id_asset_element) elmnt_grp "
        );

        tntdb::Row row = st.selectRow();

        int r = 0;
        row[0].get(r);
        LOG_END;
        return r;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
}

int
select_group_names (tntdb::Connection& conn,
                    uint32_t id,
                    std::function<void(const tntdb::Row&)> cb)
{
    LOG_START;
    log_debug("id: %" PRIu32, id);
    try{
        tntdb::Statement st = conn.prepare(
            " SELECT "
            "   v2.name "
            " FROM v_bios_asset_group_relation v1 "
            " JOIN v_bios_asset_element v2 "
            "   ON v1.id_asset_group=v2.id "
            " WHERE v1.id_asset_element=:id "
        );

        tntdb::Result res = st.set("id", id).select();

        for (const auto& r: res) {
            cb(r);
        }
        LOG_END;
        return 0;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
}

int
select_group_names (tntdb::Connection& conn,
                    uint32_t id,
                    std::vector<std::string>& out)
{
    std::function<void(const tntdb::Row&)> func = \
        [&out](const tntdb::Row& r)
        {
            std::string name;
            r["name"].get(name);
            out.push_back(name);
        };
    return select_group_names(conn, id, func);
}

int
select_v_web_asset_power_link_src_byId (tntdb::Connection& conn,
                                        uint32_t id,
                                        row_cb_f& cb)
{
    LOG_START;
    log_debug("id: %" PRIu32, id);
    try{
        tntdb::Statement st = conn.prepare(
            " SELECT "
            "   v.id_link, "
            "   v.id_asset_element_src, "
            "   v.src_name, "
            "   v.id_asset_element_dest, "
            "   v.dest_name, "
            "   v.src_out, "
            "   v.dest_in "
            " FROM v_web_asset_link v "
            " WHERE v.id_asset_element_dest=:id "
            " AND v.link_name = 'power chain' "
        );

        tntdb::Result res = st.set("id", id).select();

        for (const auto& r: res) {
            cb(r);
        }
        LOG_END;
        return 0;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
}

int
select_asset_ext_attribute_by_keytag (tntdb::Connection &conn,
                                      const std::string &keytag,
                                      const std::set<uint32_t> &element_ids,
                                      std::function< void( const tntdb::Row& ) > &cb)
{
    LOG_START;
    try{
        std::string inlist;
        for( const auto &id : element_ids ) {
            inlist += ",";
            inlist += std::to_string(id);
        }
        tntdb::Statement st = conn.prepare(
            " SELECT "
            "   id_asset_ext_attribute, keytag, value, "
            "   id_asset_element, read_only "
            " FROM "
            "   v_bios_asset_ext_attributes "
            " WHERE keytag = :keytag" +
            ( element_ids.empty() ? "" : " AND id_asset_element in (" + inlist.substr(1) + ")" )
        );
        tntdb::Result rows = st.set("keytag", keytag ).select();
        for( const auto &row: rows ) cb( row );
        LOG_END;
        return 0;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
}

int
select_ext_rw_attributes_keytags (tntdb::Connection& conn,
                                  std::function<void(const tntdb::Row&)>& cb)
{
    LOG_START;
    try{
        tntdb::Statement st = conn.prepare(
            " SELECT"
            "   DISTINCT(keytag)"
            " FROM"
            "   v_bios_asset_ext_attributes"
            " WHERE "
            "   read_only = 0"
            " ORDER BY keytag "
        );

        tntdb::Result res = st.select();

        for (const auto& r: res) {
            cb(r);
        }
        LOG_END;
        return 0;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
}

int
select_ext_attributes_cb (tntdb::Connection &conn,
                       uint32_t asset_id,
                       std::function<void(const tntdb::Row&)> cb)
{
    try {
        // Can return more than one row
        tntdb::Statement st_extattr = conn.prepare(
            " SELECT"
            "   v.keytag, v.value, v.read_only"
            " FROM"
            "   v_bios_asset_ext_attributes v"
            " WHERE v.id_asset_element = :asset_id"
        );

        tntdb::Result result = st_extattr.set("asset_id", asset_id).
                                          select();
        log_debug("[v_bios_asset_ext_attributes]: were selected %" PRIu32 " rows", result.size());

        // Go through the selected extra attributes
        for ( const auto &row: result ) {
            cb(row);
        }
        return 0;
    }
    catch (const std::exception &e) {
        log_error ("select_ext: %s", e.what());
        return -1;
    }
}

int
select_asset_element_basic_cb (tntdb::Connection &conn,
                             const std::string &asset_name,
                             std::function<void(const tntdb::Row&)> cb)
{
    log_debug ("asset_name = %s", asset_name.c_str());
    try{
        tntdb::Statement st = conn.prepare(
            " SELECT"
            "   v.id, v.name, v.id_type, v.type_name,"
            "   v.subtype_id, v.id_parent,"
            "   v.id_parent_type, v.status,"
            "   v.priority, v.asset_tag, v.parent_name "
            " FROM"
            "   v_web_element v"
            " WHERE :name = v.name"
        );

        tntdb::Row row = st.set("name", asset_name).
                            selectRow();
        log_debug("[v_web_element]: were selected %" PRIu32 " rows", 1);
        cb(row);
        return 0;
    }
    catch (const tntdb::NotFound &e) {
        log_info ("end: asset '%s' not found", asset_name.c_str());
        return -1;
    }
    catch (const std::exception &e) {
        log_error ("Cannot select basic asset info: %s", e.what());
        return -1;
    }
}

int
select_assets_cb (tntdb::Connection &conn,
                  std::function<void(const tntdb::Row&)> cb)
{
    try{
        tntdb::Statement st = conn.prepare(
            " SELECT "
            "   v.name, "
            "   v.id,  "
            "   v.id_type,  "
            "   v.id_subtype,  "
            "   v.id_parent,  "
            "   v.parent_name,  "
            "   v.status,  "
            "   v.priority,  "
            "   v.asset_tag  "
            " FROM v_bios_asset_element v "
            );

        tntdb::Result res = st.select ();
        log_debug("[v_bios_asset_element]: were selected %zu rows", res.size());

        for (const auto& r: res) {
            cb(r);
        }
        return 0;
    }
    catch (const tntdb::NotFound &e) {
        log_debug("[v_bios_asset_element]: asset not found");
        return -1;
    }
    catch (const std::exception &e) {
        log_error ("[v_bios_asset_element]: error '%s'", e.what());
        return -1;
    }
}

// TODO: this function is probably not necessary, refactor and remove
db_reply_t
select_monitor_device_type_id (tntdb::Connection &conn,
                               const char *device_type_name)
{
    LOG_START;

    log_debug ("  device_type_name = %s", device_type_name);

    db_reply_t ret = db_reply_new();

    try{
        tntdb::Statement st = conn.prepare(
            " SELECT"
            "   v.id"
            " FROM"
            "   v_bios_device_type v"
            " WHERE v.name = :name"
        );

        tntdb::Value val = st.set("name", device_type_name).
                              selectValue();
        log_debug ("[t_bios_monitor_device]: was selected 1 rows");

        val.get(ret.item);
        ret.status = 1;
        LOG_END;
        return ret;
    }
    catch (const tntdb::NotFound &e){
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_NOTFOUND;
        ret.msg        = e.what();
        log_info ("end: discovered device type was not found with '%s'", e.what());
        return ret;
    }
    catch (const std::exception &e) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_INTERNAL;
        ret.msg        = e.what();
        LOG_END_ABNORMAL (e);
        return ret;
    }
}

db_reply <db_web_basic_element_t>
select_asset_element_web_byId (tntdb::Connection &conn,
                               uint32_t element_id)
{
    LOG_START;
    log_debug ("element_id = %" PRIi32, element_id);

    // TODO write function new
    db_web_basic_element_t item {0, "", "", 0, 0, "", 0, 0, 0, "","",""};
    db_reply <db_web_basic_element_t> ret = db_reply_new(item);

    try{
        // Can return more than one row.
        tntdb::Statement st = conn.prepare(
            " SELECT"
            "   v.id, v.name, v.id_type, v.type_name,"
            "   v.subtype_id, v.subtype_name, v.id_parent,"
            "   v.id_parent_type, v.status,"
            "   v.priority, v.asset_tag, v.parent_name "
            " FROM"
            "   v_web_element v"
            " WHERE :id = v.id"
        );

        tntdb::Row row = st.set("id", element_id).
                            selectRow();
        log_debug ("[v_web_element]: were selected %" PRIu32 " rows", 1);

        row[0].get(ret.item.id);
        row[1].get(ret.item.name);
        row[2].get(ret.item.type_id);
        row[3].get(ret.item.type_name);
        row[4].get(ret.item.subtype_id);
        row[5].get(ret.item.subtype_name);
        row[6].get(ret.item.parent_id);
        row[7].get(ret.item.parent_type_id);
        row[8].get(ret.item.status);
        row[9].get(ret.item.priority);
        row[10].get(ret.item.asset_tag);
        row[11].get(ret.item.parent_name);

        ret.status = 1;
        LOG_END;
        return ret;
    }
    catch (const tntdb::NotFound &e) {
        ret.status        = 0;
        ret.errtype       = DB_ERR;
        ret.errsubtype    = DB_ERROR_NOTFOUND;
        ret.msg           = TRANSLATE_ME ("element with specified id was not found");
        log_info ("end: %s", ret.msg.c_str());
        return ret;
    }
    catch (const std::exception &e) {
        ret.status        = 0;
        ret.errtype       = DB_ERR;
        ret.errsubtype    = DB_ERROR_INTERNAL;
        ret.msg           = JSONIFY (e.what());
        LOG_END_ABNORMAL(e);
        return ret;
    }
}


db_reply <db_web_basic_element_t>
select_asset_element_web_byName (tntdb::Connection &conn,
                                 const char *element_name)
{
    // TODO write function new
    db_web_basic_element_t item {0, "", "", 0, 0, "", 0, 0, 0, "","",""};
    db_reply <db_web_basic_element_t> ret = db_reply_new(item);

    try {
        tntdb::Statement st = conn.prepare(
            " SELECT"
            "   v.id, v.name, v.id_type, v.type_name,"
            "   v.subtype_id, v.subtype_name, v.id_parent,"
            "   v.id_parent_type, v.status,"
            "   v.priority, v.asset_tag, v.parent_name "
            " FROM"
            "   v_web_element v"
            " WHERE :name = v.name"
        );

        tntdb::Row row = st.set ("name", element_name).selectRow ();

        row[0].get(ret.item.id);
        row[1].get(ret.item.name);
        row[2].get(ret.item.type_id);
        row[3].get(ret.item.type_name);
        row[4].get(ret.item.subtype_id);
        row[5].get(ret.item.subtype_name);
        row[6].get(ret.item.parent_id);
        row[7].get(ret.item.parent_type_id);
        row[8].get(ret.item.status);
        row[9].get(ret.item.priority);
        row[10].get(ret.item.asset_tag);
        row[11].get(ret.item.parent_name);

        ret.status = 1;
        return ret;
    }
    catch (const tntdb::NotFound &e) {
        ret.status        = 0;
        ret.errtype       = DB_ERR;
        ret.errsubtype    = DB_ERROR_NOTFOUND;
        ret.msg           = std::string ("Element with specified name = '").append (element_name).append ("' was not found.");
        return ret;
    }
    catch (const std::exception &e) {
        ret.status        = 0;
        ret.errtype       = DB_ERR;
        ret.errsubtype    = DB_ERROR_INTERNAL;
        ret.msg           = std::string ("Exception caught: '").append (e.what()).append ("'.");
        return ret;
    }
}

db_reply <std::map <std::string, std::pair<std::string, bool> >>
select_ext_attributes (tntdb::Connection &conn,
                       uint32_t element_id)
{
    LOG_START;
    log_debug ("element_id = %" PRIi32, element_id);

    std::map <std::string, std::pair<std::string, bool> > item{};
    db_reply <std::map <std::string, std::pair<std::string, bool> > > ret =
                                                    db_reply_new(item);
    try {
        // Can return more than one row
        tntdb::Statement st_extattr = conn.prepare(
            " SELECT"
            "   v.keytag, v.value, v.read_only"
            " FROM"
            "   v_bios_asset_ext_attributes v"
            " WHERE v.id_asset_element = :idelement"
        );

        tntdb::Result result = st_extattr.set("idelement", element_id).
                                          select();
        log_debug("[v_bios_asset_ext_attributes]: were selected %" PRIu32 " rows", result.size());

        // Go through the selected extra attributes
        for ( auto &row: result )
        {
            std::string keytag = "";
            row[0].get(keytag);
            std::string value = "";
            row[1].get(value);
            int read_only = 0;
            row[2].get(read_only);
            ret.item.insert (std::pair<std::string,std::pair<std::string, bool> >
                    (keytag, std::pair<std::string, bool>
                                                    (value,read_only?true:false)));
        }
        ret.status = 1;
        LOG_END;
        return ret;
    }
    catch (const std::exception &e) {
        ret.status        = 0;
        ret.errtype       = DB_ERR;
        ret.errsubtype    = DB_ERROR_INTERNAL;
        ret.msg           = e.what();
        ret.item.clear();
        LOG_END_ABNORMAL(e);
        return ret;
    }
}

int
select_ext_attributes (tntdb::Connection &conn,
                       uint32_t element_id,
                       std::map <std::string, std::pair<std::string, bool> >& out)
{
    auto dbreply = select_ext_attributes(conn, element_id);
    if (dbreply.status == 0)
        return -1;
    out = dbreply.item;
    return 0;
}

db_reply <std::vector <db_tmp_link_t>>
select_asset_device_links_to (tntdb::Connection &conn,
                              uint32_t element_id,
                              uint8_t link_type_id)
{
    LOG_START;
    log_debug ("element_id = %" PRIi32, element_id);

    std::vector <db_tmp_link_t> item{};
    db_reply <std::vector <db_tmp_link_t>> ret = db_reply_new(item);

    try {
        // Get information about the links the specified device
        // belongs to
        // Can return more than one row
        tntdb::Statement st_pow = conn.prepare(
            " SELECT"
            "   v.id_asset_element_src, v.src_out, v.dest_in, v.src_name"
            " FROM"
            "   v_web_asset_link v"
            " WHERE"
            "   v.id_asset_element_dest = :iddevice AND"
            "   v.id_asset_link_type = :idlinktype"
        );

        tntdb::Result result = st_pow.set("iddevice", element_id).
                                      set("idlinktype", link_type_id).
                                      select();
        log_debug("[v_bios_asset_link]: were selected %" PRIu32 " rows", result.size());

        // Go through the selected links
        for ( auto &row: result )
        {
            db_tmp_link_t m{0, element_id, "", "", ""};
            row[0].get(m.src_id);
            row[1].get(m.src_socket);
            row[2].get(m.dest_socket);
            row[3].get(m.src_name);

            ret.item.push_back (m);
        }
        ret.status = 1;
        LOG_END;
        return ret;
    }
    catch (const std::exception &e) {
        ret.status        = 0;
        ret.errtype       = DB_ERR;
        ret.errsubtype    = DB_ERROR_INTERNAL;
        ret.msg           = e.what();
        ret.item.clear();
        LOG_END_ABNORMAL(e);
        return ret;
    }
}

db_reply <std::map <uint32_t, std::string> >
select_asset_element_groups (tntdb::Connection &conn,
                             uint32_t element_id)
{
    LOG_START;
    log_debug ("element_id = %" PRIi32, element_id);

    std::map <uint32_t, std::string> item{};
    db_reply <std::map <uint32_t, std::string> > ret = db_reply_new(item);

    try {
        // Get information about the groups element belongs to
        // Can return more than one row
        tntdb::Statement st_gr = conn.prepare(
            " SELECT "
            "   v1.id_asset_group, v.name "
            " FROM "
            "   v_bios_asset_group_relation v1, "
            "   v_bios_asset_element v "
            " WHERE "
            "   v1.id_asset_element = :idelement AND "
            "   v.id = v1.id_asset_group "
        );

        tntdb::Result result = st_gr.set("idelement", element_id).
                                     select();

        log_debug("[v_bios_asset_group_relation]: were selected %" PRIu32 " rows", result.size());
        // Go through the selected groups
        for ( auto &row: result )
        {
            // group_id, required
            uint32_t group_id = 0;
            row["id_asset_group"].get(group_id);

            std::string group_name;
            row["name"].get(group_name);
            ret.item.emplace(group_id, group_name);
        }
        ret.status = 1;
        LOG_END;
        return ret;
    }
    catch (const std::exception &e) {
        ret.status        = 0;
        ret.errtype       = DB_ERR;
        ret.errsubtype    = DB_ERROR_INTERNAL;
        ret.msg           = e.what();
        ret.item.clear();
        LOG_END_ABNORMAL(e);
        return ret;
    }
}

db_reply <std::map <uint32_t, std::string> >
select_short_elements (tntdb::Connection &conn,
                       uint16_t type_id,
                       uint16_t subtype_id)
{
    LOG_START;
    log_debug ("  type_id = %" PRIi16, type_id);
    log_debug ("  subtype_id = %" PRIi16, subtype_id);
    std::map <uint32_t, std::string> item{};
    db_reply <std::map <uint32_t, std::string> > ret = db_reply_new(item);

    std::string query;
    if ( subtype_id == 0 )
    {
        query = " SELECT "
                "   v.name, v.id "
                " FROM "
                "   v_bios_asset_element v "
                " WHERE "
                "   v.id_type = :typeid ";
    }
    else
    {
        query = " SELECT "
                "   v.name, v.id "
                " FROM "
                "   v_bios_asset_element v "
                " WHERE "
                "   v.id_type = :typeid AND "
                "   v.id_subtype = :subtypeid ";
    }
    try{
        // Can return more than one row.
        tntdb::Statement st = conn.prepare(query);

        tntdb::Result result;
        if ( subtype_id == 0 )
        {
            result = st.set("typeid", type_id).
                    select();
        } else {
            result = st.set("typeid", type_id).
                    set("subtypeid", subtype_id).
                    select();
        }

        // Go through the selected elements
        for (auto const& row: result) {
            std::string name;
            row[0].get(name);
            uint32_t id = 0;
            row[1].get(id);
            ret.item.insert(std::pair<uint32_t, std::string>(id, name));
        }
        ret.status = 1;
        LOG_END;
        return ret;
    }
    catch (const std::exception &e) {
        ret.status        = 0;
        ret.errtype       = DB_ERR;
        ret.errsubtype    = DB_ERROR_INTERNAL;
        ret.msg           = e.what();
        ret.item.clear();
        LOG_END_ABNORMAL(e);
        return ret;
    }
}

db_reply <std::vector<db_a_elmnt_t>>
select_asset_elements_by_type (tntdb::Connection &conn,
                               uint16_t type_id,
                               std::string status)
{

    std::vector<db_a_elmnt_t> item{};
    db_reply <std::vector<db_a_elmnt_t>> ret = db_reply_new(item);

    try{
        // Can return more than one row.
        tntdb::Statement st = conn.prepare(
            " SELECT"
            "   v.name , v.id_parent, v.status, v.priority, v.id, v.id_subtype"
            " FROM"
            "   v_bios_asset_element v"
            " WHERE v.id_type = :typeid AND"
            "   v.status = :vstatus "
        );

        tntdb::Result result = st.set("typeid", type_id).
                                  set("vstatus", status).
                                  select();
        log_trace("[v_bios_asset_element]: were selected %" PRIu32 " rows",
                                                            result.size());

        // Go through the selected elements
        for ( auto &row: result )
        {
            db_a_elmnt_t m{0,"","",0,5,0,0,""};

            row[0].get(m.name);
            assert ( !m.name.empty() );  // database is corrupted

            row[1].get(m.parent_id);
            row[2].get(m.status);
            row[3].get(m.priority);
            row[4].get(m.id);
            row[5].get(m.subtype_id);

            ret.item.push_back(m);
        }
        ret.status = 1;
        return ret;
    }
    catch (const std::exception &e) {
        ret.status        = 0;
        ret.errtype       = DB_ERR;
        ret.errsubtype    = DB_ERROR_INTERNAL;
        ret.msg           = e.what();
        ret.item.clear();
        log_error(e.what());
        return ret;
    }
}

db_reply <std::set <std::pair<uint32_t, uint32_t>>>
select_links_by_container (tntdb::Connection &conn,
                           uint32_t element_id,
                           std::string status)
{
    log_trace ("  links are selected for element_id = %" PRIi32, element_id);
    uint8_t linktype = INPUT_POWER_CHAIN;

    //      all powerlinks are included into "resultpowers"
    std::set <std::pair<uint32_t ,uint32_t>> item{};
    db_reply <std::set<std::pair<uint32_t ,uint32_t>>> ret = db_reply_new(item);

    try{
        // v_bios_asset_link are only devices,
        // so there is no need to add more constrains
        tntdb::Statement st = conn.prepare(
            " SELECT"
            "   v.id_asset_element_src,"
            "   v.id_asset_element_dest"
            " FROM"
            "   v_bios_asset_link AS v,"
            "   v_bios_asset_element_super_parent AS v1,"
            "   v_bios_asset_element_super_parent AS v2"
            " WHERE"
            "   v.id_asset_link_type = :linktypeid AND"
            "   v.id_asset_element_dest = v2.id_asset_element AND"
            "   v.id_asset_element_src = v1.id_asset_element AND"
            "   ("
            "       ( :containerid IN (v2.id_parent1, v2.id_parent2 ,v2.id_parent3,"
            "                          v2.id_parent4, v2.id_parent5, v2.id_parent6,"
            "                          v2.id_parent7, v2.id_parent8, v2.id_parent9,"
            "                          v2.id_parent10) AND v1.status = :vstatus AND v2.status = :vstatus) OR"
            "       ( :containerid IN (v1.id_parent1, v1.id_parent2 ,v1.id_parent3,"
            "                          v1.id_parent4, v1.id_parent5, v1.id_parent6,"
            "                          v1.id_parent7, v1.id_parent8, v1.id_parent9,"
            "                          v1.id_parent10) AND v1.status = :vstatus AND v2.status = :vstatus)"
            "   )"
        );

        // can return more than one row
        tntdb::Result result = st.set("containerid", element_id).
                                  set("linktypeid", linktype).
                                  set("vstatus", status).
                                  select();
        log_trace("[t_bios_asset_link]: were selected %" PRIu32 " rows",
                                                         result.size());
        // debug helper
        std::vector <std::string> inactive = list_devices_with_status (conn,"nonactive");
        log_trace ("Inactive devices omitted:");
        for (auto dev : inactive) {
            log_trace ("\t- %s", dev.c_str ());
        }

        // Go through the selected links
        for ( auto &row: result )
        {
            // id_asset_element_src, required
            uint32_t id_asset_element_src = 0;
            row[0].get(id_asset_element_src);
            assert ( id_asset_element_src );

            // id_asset_element_dest, required
            uint32_t id_asset_element_dest = 0;
            row[1].get(id_asset_element_dest);
            assert ( id_asset_element_dest );

            ret.item.insert(std::pair<uint32_t, uint32_t>(id_asset_element_src, id_asset_element_dest));
        } // end for
        ret.status = 1;
        return ret;
    }
    catch (const std::exception &e) {
        ret.status     = 0;
        ret.errtype    = DB_ERR;
        ret.errsubtype = DB_ERROR_INTERNAL;
        ret.msg        = e.what();
        log_error (e.what());
        return ret;
    }
}

// returns vector with either active or inactive devices
std::vector <std::string>
list_devices_with_status (tntdb::Connection &conn, std::string status)
{
    std::vector <std::string> asset_list;
    try {
        tntdb::Statement st = conn.prepare(
            " SELECT"
            "   v.name, v.id_subtype"
            " FROM"
            "   v_bios_asset_element v"
            " WHERE v.status = :vstatus "
        );

        tntdb::Result result = st.set("vstatus", status).select();
        log_trace("[v_bios_asset_element]: were selected %" PRIu32 " rows",
                                                            result.size());
        for (auto &row : result) {
            std::string device;
            row [0].get (device);
            asset_list.push_back (device);
        }

    }
    catch (const std::exception &e) {
        throw std::runtime_error("Reading from DB failed.");
    }
    return asset_list;
}

std::vector <std::string>
list_power_devices_with_status (tntdb::Connection &conn, const std::string & status)
{
    std::vector <std::string> asset_list;
    try {
        tntdb::Statement st = conn.prepareCached(
            " SELECT"
            "   v.name, v.id_subtype"
            " FROM"
            "   t_bios_asset_element v"
            " WHERE v.id_subtype IN "
                "(SELECT id_asset_device_type FROM t_bios_asset_device_type "
                "WHERE name IN ('epdu', 'sts', 'ups', 'pdu', 'genset')) "
            " AND v.status = :vstatus "
        );

        tntdb::Result result = st.set("vstatus", status).select();
        log_trace("[t_bios_asset_element]: were selected %" PRIu32 " rows",
                                                            result.size());
        for (auto &row : result) {
            std::string device;
            row [0].get (device);
            asset_list.push_back (device);
        }

    }
    catch (const std::exception &e) {
        throw std::runtime_error("Reading from DB failed.");
    }
    return asset_list;
}

std::vector <std::string>
list_power_devices_with_status (const std::string & status)
{
    tntdb::Connection conn = tntdb::connectCached(DBConn::url);

    std::vector <std::string> asset_list;
    asset_list = list_power_devices_with_status(conn, status);
    return asset_list;
}

int
get_active_power_devices (tntdb::Connection &conn)
{
    int count = 0;
    try {
        tntdb::Statement st = conn.prepare (
            "SELECT COUNT(*) AS CNT FROM t_bios_asset_element "
            "WHERE id_subtype IN "
                "(SELECT id_asset_device_type FROM t_bios_asset_device_type "
                "WHERE name IN ('epdu', 'sts', 'ups', 'pdu', 'genset')) "
            "AND status = 'active';"
        );

        tntdb::Row row = st.selectRow ();

        row [0].get (count);
        log_debug ("[get_active_power_devices]: detected %d active power devices", count);
    }
    catch (const std::exception &e)
    {
        log_error ("exception caught %s when getting count of active power devices", e.what ());
        return 0;
    }

    return count;
}

std::string
get_status_from_db_helper (const std::string &element_name)
{
    tntdb::Connection conn = tntdb::connectCached(DBConn::url);
    std::string status = get_status_from_db (conn, element_name);
    return status;
}


std::string
get_status_from_db (tntdb::Connection conn,
                    const std::string &element_name)
{
    try {
        log_debug("get_status_from_db: getting status for asset %s", element_name.c_str());
        tntdb::Statement st = conn.prepare(
            " SELECT v.status "
            " FROM v_bios_asset_element v "
            " WHERE v.name=:vname ;"
            );

        tntdb::Row row = st.set ("vname", element_name).selectRow ();
        log_debug("get_status_from_db: [v_bios_asset_element]: were selected %zu rows", row.size());
        if (row.size() == 1) {
            std::string ret;
            row [0].get (ret);
            return ret;
        } else {
            return "unknown";
        }
    }
    catch (const tntdb::NotFound &e) {
        log_debug("get_status_from_db: [v_bios_asset_element]: %s asset not found", element_name.c_str ());
        return "unknown";
    }
    catch (const std::exception &e) {
        log_error ("get_status_from_db: [v_bios_asset_element]: error '%s'", e.what());
        return "unknown";
    }
}

db_reply <std::map <int, std::string> >
select_daisy_chain (tntdb::Connection &conn, const std::string &asset_id)
{
    LOG_START;
    log_debug ("  asset_id = %s", asset_id.c_str());
    std::map <int, std::string> item{};
    db_reply <std::map <int, std::string> > ret = db_reply_new(item);

    std::string query = R"EOF(
select ae_name_out.name, aea_daisychain.value as daisy_chain
    from t_bios_asset_ext_attributes aea_daisychain join t_bios_asset_element ae_name_out on aea_daisychain.id_asset_element = ae_name_out.id_asset_element
    where aea_daisychain.keytag = 'daisy_chain' and aea_daisychain.id_asset_element in
    (
        select aea_name.id_asset_element from t_bios_asset_ext_attributes aea_name where aea_name.value in
        (
            select aea_ip.value
                from t_bios_asset_ext_attributes aea_ip
                where aea_ip.id_asset_element =
                (
                    select ae_name_in.id_asset_element
                        from t_bios_asset_element ae_name_in
                        where ae_name_in.name = :asset_id
                ) and aea_ip.keytag like 'ip.%'
        )
    )
)EOF";
    try{
        // Can return more than one row.
        tntdb::Statement st = conn.prepare(query);
        tntdb::Result result = st.set("asset_id", asset_id).select();

        // Go through the selected elements
        for (auto const& row: result) {
            int daisy_chain;
            std::string name;
            row[0].get(name);
            row[1].get(daisy_chain);
            ret.item.emplace(daisy_chain, name);
        }
        ret.status = 1;
        LOG_END;
        return ret;
    }
    catch (const std::exception &e) {
        ret.status        = 0;
        ret.errtype       = DB_ERR;
        ret.errsubtype    = DB_ERROR_INTERNAL;
        ret.msg           = e.what();
        ret.item.clear();
        LOG_END_ABNORMAL(e);
        return ret;
    }
}

} // namespace

//  --------------------------------------------------------------------------
//  Self test of this class

// If your selftest reads SCMed fixture data, please keep it in
// src/selftest-ro; if your test creates filesystem objects, please
// do so under src/selftest-rw.
// The following pattern is suggested for C selftest code:
//    char *filename = NULL;
//    filename = zsys_sprintf ("%s/%s", SELFTEST_DIR_RO, "mytemplate.file");
//    assert (filename);
//    ... use the "filename" for I/O ...
//    zstr_free (&filename);
// This way the same "filename" variable can be reused for many subtests.
#define SELFTEST_DIR_RO "src/selftest-ro"
#define SELFTEST_DIR_RW "src/selftest-rw"

void
fty_common_db_asset_test (bool verbose)
{
    printf (" * fty_common_db_asset: ");

    printf ("not tests \n");
}
