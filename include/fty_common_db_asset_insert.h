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

#pragma once

#include "fty_common_db_defs.h"
#include <fty_common_asset_types.h>
#include <inttypes.h>
#include <tntdb/connect.h>

namespace DBAssetsInsert {

// insert_into_asset_ext_attribute: insert given ext attribute into t_bios_asset_ext_attributes
// returns error if input params are unacceptable or something went wrong
db_reply_t insert_into_asset_ext_attribute(
    tntdb::Connection& conn, const char* value, const char* keytag, uint32_t asset_element_id, bool read_only);

// insert_into_asset_ext_attribute: multi value insert for extended attributes
// returns error if any insert wasn't successful
db_reply_t insert_into_asset_ext_attributes(
    tntdb::Connection& conn, uint32_t element_id, zhash_t* attributes, bool read_only, std::string& err);

//////////////////////////////////////////////////////////////////////////////
// insert_asset_element_into_asset_group: insert group<->asset relation
// returns error if input params are unacceptable or insert went wrong
db_reply_t insert_asset_element_into_asset_group(tntdb::Connection& conn, uint32_t group_id, uint32_t asset_element_id);

// insert_element_into_groups: insert group<->asset relation for each specified group
// returns error if input params are unacceptable or any insert went wrong
db_reply_t insert_element_into_groups(
    tntdb::Connection& conn, std::set<uint32_t> const& groups, uint32_t asset_element_id);
///////////////////////////////////////////////////////////////////////////
// insert_into_asset_link: insert powerlink info
// returns error if input params are unacceptable or insert went wrong
db_reply_t insert_into_asset_link(tntdb::Connection& conn, uint32_t asset_element_src_id,
    uint32_t asset_element_dest_id, uint8_t link_type_id, const char* src_out, const char* dest_in);

// insert_into_asset_links: insert info about more powerlinks
// returns error if input params are unacceptable or any insert went wrong
db_reply_t insert_into_asset_links(tntdb::Connection& conn, std::vector<link_t> const& links);

// insert_into_asset_links: insert info about more powerlinks
// returns error if input params are unacceptable or any insert went wrong
db_reply_t insert_into_new_asset_links(tntdb::Connection& conn, std::vector<new_link_t> const& links);

//////////////////////////////////////////////////////////////////////////
// insert_into_asset_element: insert info about an asset
// returns error if input params are unacceptable or insert went wrong
db_reply_t insert_into_asset_element(tntdb::Connection& conn, const char* element_name, uint16_t element_type_id,
    uint32_t parent_id, const char* status, uint16_t priority, uint16_t subtype_id, const char* asset_tag, bool update);

//////////////////////////////////////////////////////////////////////////
// insert_into_monitor_asset_relation: insert monitor_id<->element_id relation
// returns error if input params are unacceptable or insert went wrong
db_reply_t insert_into_monitor_asset_relation(tntdb::Connection& conn, uint16_t monitor_id, uint32_t element_id);

// insert_into_monitor_device: insert name<->device_type relation
// returns error if insert went wrong
db_reply_t insert_into_monitor_device(tntdb::Connection& conn, uint16_t device_type_id, const char* device_name);
} // namespace DBAssetsInsert
