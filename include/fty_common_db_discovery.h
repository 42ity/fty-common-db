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

#define REQUEST_WHERE_CANDIDATE_CONFIG " WHERE config.id_asset_element = :asset_id AND config.is_working = TRUE AND config.is_enabled = TRUE"
#define REQUEST_WHERE_ALL_CONFIG       " WHERE config.id_asset_element =: asset_id"

typedef std::vector<std::pair< std::string, nutcommon::DeviceConfiguration>> DeviceConfigurationIdList;
typedef std::tuple<int, std::string, std::string, std::string, std::map<std::string, std::string>, std::vector<std::string>> ConfigurationInfo;
typedef std::vector<ConfigurationInfo> ConfigurationInfoList;

/* @function get_candidate_config Get first candidate configuration of an asset
 * @param asset_name The asset name to get configuration
 * @param config_id [out] The return configuration id
 * @param device_config [out] The return configuration of the asset
 * @return {integer} 0 if no error else < 0
 */
int get_candidate_config (std::string asset_name, std::string& config_id, nutcommon::DeviceConfiguration& device_config);

/**
 * @function get_candidate_config_list Get all candidate configurations of an asset
 * @param asset_name The asset name to get configurations
 * @param device_config_id_list [out] The return configuration list of the asset
 * @return {integer} 0 if no error else < 0
 */
int get_candidate_config_list (std::string asset_name, DeviceConfigurationIdList& device_config_id_list);

/**
 * @function get_all_config_list Get all configurations of an asset
 * @param asset_name The asset name to get configuration
 * @param device_config_id_list [out] The return configuration list of the asset
 * @return {integer} 0 if no error else < 0
 */
int get_all_config_list (std::string asset_name, DeviceConfigurationIdList& device_config_id_list);

/**
 * @function get_config_working Get working value of a configuration
 * @param config_id The configuration id
 * @param working_value [out] The return working value
 * @return {integer} 0 if no error else < 0
 */
int get_config_working (std::string config_id, bool &working_value);

/**
 * @function set_config_working Change working value of a configuration
 * @param config_id The configuration id
 * @param working_value The new working value
 * @return {integer} 0 if no error else < 0
 */
int set_config_working (std::string config_id, bool working_value);

/**
 * @function modify_config_priorities Change priorities of configuration list for an asset
 * @param asset_name The asset name to change priorities of configuration list
 * @param configuration_id_list The list of configuration for priorities (first in the list is the highest priority)
 * @return {integer} 0 if no error else < 0
 */
int modify_config_priorities (std::string asset_name, std::vector<std::string>& configuration_id_list);

/**
 * @function insert_config Insert a new configuration for an asset
 * @param asset_name The asset name to add  new configuration
 * @param is_working Value of is_working attribute
 * @param is_enabled Value of is_enabled attribute
 * @param key_value_asset_list The list of key values to add in the asset configuration attribute table
 * @return {integer} configuration id if no error else < 0
 */
int insert_config (std::string asset_name, int config_type, bool is_working, bool is_enabled, std::map<std::string, std::string>& key_value_asset_list);

/**
 * @function remove_config Remove a configuration from database
 * @param config_id The configuration id to remove
 * @return {integer} 0 if no error else < 0
 */
int remove_config (int config_id);

/**
 * @function get_configuration_from_type Get specific configuration information for each configuration type
 * @param config_type The configuration type
 * @param [out] config_info The returned configuration information
 * @return {integer} 0 if no error else < 0
 */
int get_configuration_from_type (/*int config_type, */ConfigurationInfoList& config_info_list);

/**
 * @function get_config_type_from_info Get configuration type from specific configuration information
  * @param config_info The specific configuration information
  * @return {integer} Return configuration type
 */
int get_config_type_from_info(ConfigurationInfo& config_info);

/**
 * @function get_config_name_from_info Get configuration name from specific configuration information
  * @param config_info The specific configuration information
  * @return {string} Return configuration name
 */
std::string get_config_name_from_info(ConfigurationInfo& config_info);

/**
 * @function get_config_driver_from_info Get configuration driver from specific configuration information
  * @param config_info The specific configuration information
  * @return {string} Return configuration name
 */
std::string get_config_driver_from_info(ConfigurationInfo& config_info);

/**
 * @function get_config_port_from_info Get configuration port from specific configuration information
  * @param config_info The specific configuration information
  * @return {string} Return configuration port
 */
std::string get_config_port_from_info(ConfigurationInfo& config_info);

/**
 * @function get_config_default_key_value_list_from_info Get configuration default key value list from specific configuration information
  * @param config_info The specific configuration information
  * @return {map} Return configuration default key value list
 */
std::map<std::string, std::string> get_config_default_key_value_list_from_info(ConfigurationInfo& config_info);

/**
 * @function get_config_document_type_list_from_info Get configuration document type list from specific configuration information
  * @param config_info The specific configuration information
  * @return {list} Return configuration document type list
 */
std::vector<std::string> get_config_document_type_list_from_info(ConfigurationInfo& config_info);

} // namespace

void fty_common_db_discovery_test (bool verbose);

#endif //namespace
#endif // FTY_COMMON_DB_DISCOVERY_H_INCLUDED
