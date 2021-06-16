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

#pragma once

#include "fty_common_db_defs.h"
#include <tntdb/connect.h>

namespace DBAssetsDelete {

// delete_asset_link: delete src<->dst link
// returns error if input params are unacceptable or error happened during delete
db_reply_t delete_asset_link(tntdb::Connection& conn, uint32_t asset_element_id_src, uint32_t asset_element_id_dest);

// delete_asset_links_to: delete all links which have given asset as 'dest'
// returns error if input params are unacceptable or error happened during delete
db_reply_t delete_asset_links_to(tntdb::Connection& conn, uint32_t asset_device_id);

///////////////////////////////////////////////////////////////////////////
// delete_asset_group_links: delete all data about a group
// returns error if input params are unacceptable or error happened during delete
db_reply_t delete_asset_group_links(tntdb::Connection& conn, uint32_t asset_group_id);

////////////////////////////////////////////////////////////////////////////
// delete_asset_ext_attribute: delete given ext attribute of a given asset
// returns error if input params are unacceptable or error happened during delete
db_reply_t delete_asset_ext_attribute(tntdb::Connection& conn, const char* keytag, uint32_t asset_element_id);

// delete_asset_ext_attributes_with_ro: delete all ext attributes of given asset with given 'read_only' status
// returns error if input params are unacceptable or error happened during delete
db_reply_t delete_asset_ext_attributes_with_ro(tntdb::Connection& conn, uint32_t asset_element_id, bool read_only);

////////////////////////////////////////////////////////////////////////////
// delete_asset_element: delete asset from t_bios_asset_element
// returns error if input params are unacceptable or error happened during delete
db_reply_t delete_asset_element(tntdb::Connection& conn, uint32_t asset_element_id);

/////////////////////////////////////////////////////////////////////////////
// delete_asset_element_from_asset_groups: delete asset from all group
// returns error if input params are unacceptable or error happened during delete
db_reply_t delete_asset_element_from_asset_groups(tntdb::Connection& conn, uint32_t asset_element_id);

// delete_asset_element_from_asset_groups: delete asset from specified group
// returns error if input params are unacceptable or error happened during delete
db_reply_t delete_asset_element_from_asset_group(
    tntdb::Connection& conn, uint32_t asset_group_id, uint32_t asset_element_id);

///////////////////////////////////////////////////////////////////////////
// delete_monitor_asset_relation_by_a: delete given asset from t_bios_monitor_asset_relation
// returns error if input params are unacceptable or error happened during delete
db_reply_t delete_monitor_asset_relation_by_a(tntdb::Connection& conn, uint32_t id);
} // namespace DBAssetsDelete
