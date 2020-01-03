/*  =========================================================================
    fty_common_db_discovery - Discovery configuration functions

    Copyright (C) 2014 - 2019 Eaton

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

#ifndef FTY_COMMON_DB_DISCOVERY_H_INCLUDED
#define FTY_COMMON_DB_DISCOVERY_H_INCLUDED

// Note: Consumers MUST be built with C++11 or newer standard due to this:
#include "fty_common_db_defs.h"
#include "fty_common_library.h"
#include "fty_common_nut_utils.h"

#include <iomanip>
#include <map>
#include <tuple>
#include <iostream>

#ifdef __cplusplus

namespace DBAssetsDiscovery {

struct DeviceConfigurationInfo {
    size_t id;
    nutcommon::DeviceConfiguration attributes;
    std::set<secw::Id> secwDocumentIdList;
};

using DeviceConfigurationInfos = std::vector<DeviceConfigurationInfo>;

struct DeviceConfigurationInfoDetail {
    size_t id;
    std::string prettyName;
    nutcommon::DeviceConfiguration defaultAttributes;
    std::set<secw::Id> secwDocumentIdList;
    std::set<std::string> secwDocumentTypes;
};

using DeviceConfigurationInfoDetails = std::vector<DeviceConfigurationInfoDetail>;

#if 0
/**
 * @function get_candidate_config Get first candidate configuration of an asset
 * @param conn The connection to the database
 * @param asset_name The asset name to get configuration
 * @param config_id [out] The return configuration id
 * @param device_config [out] The return configuration of the asset
 * @return {integer} 0 if no error else < 0
 */
int get_candidate_config (tntdb::Connection& conn, const std::string& asset_name, std::string &config_id, nutcommon::DeviceConfiguration& device_config)
#endif

/**
 * @function get_candidate_config_list Get candidate configuration list of an asset
 * @param conn The connection to the database
 * @param asset_name The asset name to get configuration
 * @return The return configuration list of the asset
 */
DeviceConfigurationInfos get_candidate_config_list (tntdb::Connection& conn, const std::string& asset_name);

/**
 * @function get_all_config_list Get all configuration list of an asset
 * @param conn The connection to the database
 * @param asset_name The asset name to get configuration
 * @return The return configuration list of the asset
 */
DeviceConfigurationInfos get_all_config_list (tntdb::Connection& conn, const std::string& asset_name);

/**
 * @function is_config_working Get working value of a configuration
 * @param conn The connection to the database
 * @param config_id The configuration id
 * @return The return working value
 */
bool is_config_working (tntdb::Connection& conn, const size_t config_id);

/**
 * @function set_config_working Change working value of a configuration
 * @param conn The connection to the database
 * @param config_id The configuration id
 * @param working_value The new working value
 */
void set_config_working (tntdb::Connection& conn, const size_t config_id, const bool working_value);

/**
 * @function modify_config_priorities Change priorities of configuration list for an asset
 * @param conn The connection to the database
 * @param asset_name The asset name to change priorities of configuration list
 * @param configuration_id_list The list of configuration for priorities (first in the list is the highest priority)
 */
void modify_config_priorities (tntdb::Connection& conn, const std::string& asset_name, const std::vector<size_t>& configuration_id_list);

/**
 * @function insert_config Insert a new configuration for an asset
 * @param conn The connection to the database
 * @param asset_name The asset name to add new configuration
 * @param is_working Value of is_working attribute
 * @param is_enabled Value of is_enabled attribute
 * @param secw_document_id_list Security document id list of new configuration
 * @param key_value_asset_list The list of key values to add in the asset configuration attribute table
 * @return Configuration id in database.
 */
size_t insert_config (tntdb::Connection& conn, const std::string& asset_name, const size_t config_type,
                      const bool is_working, const bool is_enabled,
                      const std::set<secw::Id>& secw_document_id_list,
                      const nutcommon::DeviceConfiguration& key_value_asset_list);
/**
 * @function remove_config Remove a configuration from database
 * @param conn The connection to the database
 * @param config_id The configuration id to remove
 */
void remove_config (tntdb::Connection& conn, const size_t config_id);

/**
 * @function get_all_configuration_types Get specific configuration detailed information for each configuration type
 * @param conn The connection to the database
 * @return the specific configuration detailed information for each configuration type
 */
DeviceConfigurationInfoDetails get_all_configuration_types (tntdb::Connection& conn);

} // namespace

void fty_common_db_discovery_test (bool verbose);

#endif //namespace
#endif // FTY_COMMON_DB_DISCOVERY_H_INCLUDED
