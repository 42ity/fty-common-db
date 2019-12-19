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

/*
@header
    fty_common_db_discovery - Discovery configuration functions
@discuss
@end
*/

#include "fty_common_db_classes.h"
#include <fty_common_macros.h>

#include <fty_common.h>
#include <fty_common_nut_utils.h>

#include <assert.h>

namespace DBAssetsDiscovery {

/**
 * @function get_asset_id get asset id from asset name
 * @param conn The connection to the database
 * @param asset_name The asset name to search
 * @return {integer} the asset id found if success else < 0
 */
int64_t get_asset_id (tntdb::Connection conn, std::string asset_name)
{
    try
    {
        int64_t id = 0;
        tntdb::Statement st = conn.prepareCached(
                " SELECT id_asset_element"
                " FROM"
                "   t_bios_asset_element"
                " WHERE name = :asset_name"
                );

        tntdb::Row row = st.set("asset_name", asset_name).selectRow();
        row[0].get(id);
        return id;
    }
    catch (const tntdb::NotFound &e) {
        log_error("Element %s not found", asset_name.c_str ());
        return -1;
    }
    catch (const std::exception &e)
    {
        log_error("Exception caught %s for element %s", e.what (), asset_name.c_str ());
        return -2;
    }
}

/**
 * @function get_database_config Request in database first candidate configuration of an asset
 * @param conn the connection to the database
 * @param request The request to send to the database
 * @param asset_id The asset id to get configuration
 * @param config_id [out] The return configuration id
 * @param device_config [out] The return configuration of the asset
 * @return {integer} 0 if no error else < 0
 */
int request_database_config (tntdb::Connection& conn, std::string request, int64_t asset_id, std::string &config_id, nutcommon::DeviceConfiguration& device_config)
{
    try {
        tntdb::Statement st = conn.prepareCached(request);
        tntdb::Result result = st.set("asset_id", asset_id).select();
        std::string config_id_in_progress;
        for (auto &row: result) {
            row["id_nut_configuration"].get(config_id);
            if (config_id_in_progress.empty()) {
                config_id_in_progress = config_id;
            }
            if (config_id_in_progress == config_id) {
                std::string keytag;
                std::string value;
                row["keytag"].get(keytag);
                row["value"].get(value);
                if (!keytag.empty()) {
                    device_config.insert(std::make_pair(keytag.c_str(), value.c_str()));
                }
                else {
                    return -1;
                }
            }
            else {
                break;
            }
        }
        return 0;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
}

/**
 * @function get_candidate_config_ex Get first candidate configuration of an asset
 * @param conn The connection to the database
 * @param asset_name The asset name to get configuration
 * @param config_id [out] The return configuration id
 * @param device_config [out] The return configuration of the asset
 * @return {integer} 0 if no error else < 0
 */
int get_candidate_config_ex (tntdb::Connection conn, std::string asset_name, std::string &config_id, nutcommon::DeviceConfiguration& device_config)
{
    int64_t asset_id = get_asset_id(conn, asset_name);
    if (asset_id < 0) {
        log_error("Element %s not found", asset_name.c_str());
        return -2;
    }
    try {
        // Get first default configurations
        std::string request = " SELECT config.id_nut_configuration, conf_def_attr.keytag, conf_def_attr.value"
            " FROM t_bios_nut_configuration config"
            " INNER JOIN t_bios_nut_configuration_default_attribute conf_def_attr"
            " ON conf_def_attr.id_nut_configuration_type = config.id_nut_configuration_type"
            " WHERE config.id_asset_element =: asset_id AND config.is_working = TRUE AND config.is_enabled = TRUE"
              " ORDER BY config.priority ASC, config.id_nut_configuration";

        std::string default_config_id;
        nutcommon::DeviceConfiguration default_config;
        int res = request_database_config(conn, request, asset_id, default_config_id, default_config);
        if (res < 0) {
            log_error("Error during getting default configuration");
            return -3;
        }

        // Then get asset configurations
        request = " SELECT config.id_nut_configuration, conf_attr.keytag, conf_attr.value"
            " FROM t_bios_nut_configuration config"
            " INNER JOIN t_bios_nut_configuration_attribute conf_attr"
            " ON conf_attr.id_nut_configuration = config.id_nut_configuration"
            " WHERE config.id_asset_element = :asset_id AND config.is_working = TRUE AND config.is_enabled = TRUE "
            " ORDER BY config.priority ASC, config.id_nut_configuration";

        std::string asset_config_id;
        nutcommon::DeviceConfiguration asset_config;
        res = request_database_config(conn, request, asset_id, asset_config_id, asset_config);
        if (res < 0) {
            log_error("Error during getting asset configuration");
            return -4;
        }

        if (default_config_id != asset_config_id) {
            log_error("Default config id different of asset config id");
            return -5;
        }

        config_id = default_config_id;

        // Save first part of result
        device_config.insert(default_config.begin(), default_config.end());

        // Merge asset config to default config:
        // - first add new elements from asset config if not present in default config
        device_config.insert(asset_config.begin(), asset_config.end());
        // - then update default element value with asset config value if their keys are identical
        for(auto& it : asset_config) {
            device_config[it.first] = it.second;
        }
        return 0;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -6;
    }
}

/**
 * @function get_candidate_config Get first candidate configuration of an asset
 * @param asset_name The asset name to get configuration
 * @param config_id [out] The return configuration id
 * @param device_config [out] The return configuration of the asset
 * @return {integer} 0 if no error else < 0
 */
int get_candidate_config (std::string asset_name, std::string& config_id, nutcommon::DeviceConfiguration& device_config) {

    tntdb::Connection conn;
    try {
        conn = tntdb::connect(DBConn::url);
    }
    catch(...)
    {
        log_error("No connection to database");
        return -1;
    }
    return get_candidate_config_ex(conn, asset_name, config_id, device_config);
}

/**
 * @function request_database_config_list Request in database all candidate configurations of an asset
 * @param conn The connection to the database
 * @param request The request to send to the database
 * @param asset_id The asset id to get configuration
 * @param device_config_id_list [out] The return configuration list of the asset
 * @return {integer} 0 if no error else < 0
 */
int request_database_config_list (tntdb::Connection& conn, std::string request, int64_t asset_id, DeviceConfigurationIdList& device_config_id_list)
{
    try {
        tntdb::Statement st = conn.prepareCached(request);
        tntdb::Result res = st.set("asset_id", asset_id).select();
        std::string config_id_in_progress;
        nutcommon::DeviceConfiguration config;
        for (auto &row: res) {
            std::string config_id;
            row["id_nut_configuration"].get(config_id);
            if (config_id_in_progress.empty()) {
                config_id_in_progress = config_id;
            }
            if (config_id_in_progress != config_id) {
                if (!config.empty()) {
                    device_config_id_list.push_back(std::make_pair(config_id_in_progress, config));
                }
                config.erase(config.begin(), config.end());
                config_id_in_progress = config_id;
            }
            std::string keytag;
            std::string value;
            row["keytag"].get(keytag);
            row["value"].get(value);
            //std::cout << "[" << keytag << "]=" << "\"" << value << "\"" << std::endl;
            config.insert(std::make_pair(keytag.c_str(), value.c_str()));
        }
        if (!config.empty()) {
            device_config_id_list.push_back(std::make_pair(config_id_in_progress, config));
        }
        return 0;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
}

/**
 * @function get_config_list_ex Get configurations of an asset depending of where condition request
 * @param conn The connection to the database
 * @param request_where The where condition of the request to send to the database
 * @param asset_name The asset name to get configuration
 * @param device_config_id_list [out] The return configuration list of the asset
 * @return {integer} 0 if no error else < 0
 */
int get_config_list_ex (tntdb::Connection conn, std::string request_where, std::string asset_name, DeviceConfigurationIdList& device_config_id_list)
{
    int64_t asset_id = get_asset_id(conn, asset_name);
    if (asset_id < 0) {
        log_error("Element %s not found", asset_name.c_str());
        return -2;
    }
    try {
        // Get first default configurations
        std::string request = " SELECT config.id_nut_configuration, conf_def_attr.keytag, conf_def_attr.value"
            " FROM t_bios_nut_configuration config"
            " INNER JOIN t_bios_nut_configuration_default_attribute conf_def_attr"
            " ON conf_def_attr.id_nut_configuration_type = config.id_nut_configuration_type";
        request += request_where;
        request += " ORDER BY config.priority ASC, config.id_nut_configuration";

        DeviceConfigurationIdList default_config_id_list;
        int res = request_database_config_list(conn, request, asset_id, default_config_id_list);
        if (res < 0) {
            log_error("Error during getting default configurations");
            return -3;
        }

        // Then get asset configurations
        request = " SELECT config.id_nut_configuration, conf_attr.keytag, conf_attr.value"
            " FROM t_bios_nut_configuration config"
            " INNER JOIN t_bios_nut_configuration_attribute conf_attr"
            " ON conf_attr.id_nut_configuration = config.id_nut_configuration";
        request += request_where;
        request += " ORDER BY config.priority ASC, config.id_nut_configuration";

        DeviceConfigurationIdList asset_config_id_list;
        res = request_database_config_list(conn, request, asset_id, asset_config_id_list);
        if (res < 0) {
            log_error("Error during getting asset configurations");
            return -4;
        }

        // FIXME ???
        //if (default_config_id_list.size() != asset_config_id_list.size()) {
        //    log_error("Error size of asset configurations is different of size of default configurations");
        //    return -5;
        //}

        // Save first part of result
        auto it_default_config_id_list = default_config_id_list.begin();
        while (it_default_config_id_list != default_config_id_list.end()) {
            nutcommon::DeviceConfiguration config;
            std::string config_id = (*it_default_config_id_list).first;
            config.insert((*it_default_config_id_list).second.begin(), (*it_default_config_id_list).second.end());
            device_config_id_list.push_back(std::make_pair(config_id, config));
            it_default_config_id_list ++;
        }

        // Merge asset config to default config:
        auto it_asset_config_id_list = asset_config_id_list.begin();
        auto it_config_id_list = device_config_id_list.begin();
        while (it_asset_config_id_list != asset_config_id_list.end() && it_config_id_list != device_config_id_list.end()) {
            std::string config_id = (*it_config_id_list).first;
            if (config_id == (*it_asset_config_id_list).first) {
                // - first add new elements from asset config if not present in default config
                (*it_config_id_list).second.insert((*it_asset_config_id_list).second.begin(), (*it_asset_config_id_list).second.end());
                // - then update default element value with asset config value if their keys are identical
                for(auto& it : (*it_asset_config_id_list).second) {
                    (*it_config_id_list).second[it.first] = it.second;
                }
            }
            it_asset_config_id_list ++;
            it_config_id_list ++;
        }
        return 0;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -6;
    }
}

/**
 * @function get_candidate_config_list Get all candidate configurations of an asset
 * @param asset_name The asset name to get configuration
 * @param device_config_id_list [out] The return candidate configuration list of the asset
 * @return {integer} 0 if no error else < 0
 */
int get_candidate_config_list (std::string asset_name,  DeviceConfigurationIdList& device_config_id_list)
{
    tntdb::Connection conn;
    try {
        conn = tntdb::connect(DBConn::url);
    }
    catch(...)
    {
        log_error("No connection to database");
        return -1;
    }
    std::string request_where = REQUEST_WHERE_CANDIDATE_CONFIG;
    return get_config_list_ex(conn, request_where, asset_name, device_config_id_list);
}

/**
 * @function get_all_config_list Get all configurations of an asset
 * @param asset_name The asset name to get configuration
 * @param device_config_id_list [out] The return configuration list of the asset
 * @return {integer} 0 if no error else < 0
 */
int get_all_config_list (std::string asset_name, DeviceConfigurationIdList& device_config_id_list)
{
    tntdb::Connection conn;
    try {
        conn = tntdb::connect(DBConn::url);
    }
    catch(...)
    {
        log_error("No connection to database");
        return -1;
    }
    std::string request_where = REQUEST_WHERE_ALL_CONFIG;
    return get_config_list_ex(conn, request_where, asset_name, device_config_id_list);
}

/**
 * @function get_config_working_ex Get working value of a configuration
 * @param conn The connection to the database
 * @param config_id The configuration id
 * @param working_value [out] The return working value
 * @return {integer} 0 if no error else < 0
 */
int get_config_working_ex (tntdb::Connection conn, std::string config_id, bool &working_value)
{
    try {
        tntdb::Statement st = conn.prepareCached(
            " SELECT is_working"
            " FROM"
            "   t_bios_nut_configuration"
            " WHERE id_nut_configuration = :config_id"
        );

        tntdb::Row row = st.set("config_id", config_id).selectRow();
        row[0].get(working_value);
        return 0;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -2;
    }
}

/**
 * @function get_config_working Get working value of a configuration
 * @param config_id The configuration id
 * @param working_value [out] The return working value
 * @return {integer} 0 if no error else < 0
 */
int get_config_working (std::string config_id, bool &working_value)
{
    tntdb::Connection conn;
    try {
        conn = tntdb::connect(DBConn::url);
    }
    catch(...)
    {
        log_error("No connection to database");
        return -1;
    }
    return get_config_working_ex(conn, config_id, working_value);
}

/**
 * @function set_config_working_ex Change working value of a configuration
 * @param conn The connection to the database
 * @param config_id The configuration id
 * @param working_value The new working value
 * @return {integer} 0 if no error else < 0
 */
int set_config_working_ex (tntdb::Connection conn, std::string config_id, bool working_value)
{
    try {
        tntdb::Statement st = conn.prepareCached(
            " UPDATE"
            "   t_bios_nut_configuration"
            " SET"
            "   is_working = :working_value"
            " WHERE id_nut_configuration = :config_id"
        );

        st.set("config_id", config_id).set("working_value", working_value).execute();
        return 0;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -2;
    }
}

/**
 * @function set_config_working Change working value of a configuration
 * @param config_id The configuration id
 * @param working_value The new working value
 * @return {integer} 0 if no error else < 0
 */
int set_config_working (std::string config_id, bool working_value)
{
    tntdb::Connection conn;
    try {
        conn = tntdb::connect(DBConn::url);
    }
    catch(...)
    {
        log_error("No connection to database");
        return -1;
    }
    return set_config_working_ex(conn, config_id, working_value);
}

/**
 * @function modify_config_priorities Change priorities of configuration list for an asset
 * @param conn The connection to the database
 * @param asset_name The asset name to change priorities of configuration list
 * @param configuration_id_list The list of configuration for priorities (first in the list is the highest priority)
 * @return {integer} 0 if no error else < 0
 */
int modify_config_priorities_ex (tntdb::Connection conn, std::string asset_name, std::vector<std::string>& configuration_id_list)
{
    int64_t asset_id = get_asset_id(conn, asset_name);
    if (asset_id < 0) {
        log_error("Element %s not found", asset_name.c_str());
        return -2;
    }
    try {
        tntdb::Statement st = conn.prepareCached(
            " SELECT id_nut_configuration, priority"
            " FROM"
            "   t_bios_nut_configuration"
            " WHERE id_asset_element = :asset_id"
        );

        tntdb::Result result = st.set("asset_id", asset_id).select();
        std::string config_id;
        int max_priority = -1;
        int priority = -1;
        std::vector<std::string> current_config_id_list;
        for (auto &row: result) {
            row["id_nut_configuration"].get(config_id);
            row["priority"].get(priority);
            if (priority > max_priority) max_priority = priority;
            current_config_id_list.push_back(config_id);
            // Test if config present in input configuration list
            auto it = std::find(configuration_id_list.begin(), configuration_id_list.end(), config_id);
            if (it == configuration_id_list.end()) {
                log_error("Configuration id %s not found in input configuration list for %s", config_id.c_str(), asset_name.c_str());
                return -3;
            }
        }
        // Test if all input config present in database
        for (auto &configuration_id : configuration_id_list) {
            auto it = std::find(current_config_id_list.begin(), current_config_id_list.end(), configuration_id);
            if (it == current_config_id_list.end()) {
                log_error("Configuration id %s not found in database for %s", configuration_id.c_str(), asset_name.c_str());
                return -4;
            }
        }
        // Change configuration priorities
        // Note: adding a increment value for each value for avoid duplication key.
        // This increment will be removed just after updates
        max_priority ++;
        priority = max_priority;
        for (auto &configuration_id : configuration_id_list) {
            std::string request = " UPDATE t_bios_nut_configuration"
                                  " SET ";
            std::ostringstream s;
            s << "priority = " << priority;
            request += s.str();
            request += " WHERE id_asset_element = :asset_id AND id_nut_configuration = :config_id";
            st = conn.prepareCached(request);
            st.set("asset_id", asset_id).set("config_id", configuration_id).execute();
            priority ++;
        }
        // Remove increment value for priorities
        if (max_priority > 0) {
            st = conn.prepareCached(
                " UPDATE t_bios_nut_configuration"
                " SET priority = priority - :max_priority"
                " WHERE id_asset_element = :asset_id"
            );
            st.set("max_priority", max_priority).set("asset_id", asset_id).execute();
        }
        return 0;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -10;
    }
}

/**
 * @function modify_config_priorities Change priorities of configuration list for an asset
 * @param asset_name The asset name to change priorities of configuration list
 * @param configuration_id_list The list of configuration for priorities (first in the list is the highest priority)
 * @return {integer} 0 if no error else < 0
 */
int modify_config_priorities (std::string asset_name, std::vector<std::string>& configuration_id_list)
{
    tntdb::Connection conn;
    try {
        conn = tntdb::connect(DBConn::url);
    }
    catch(...)
    {
        log_error("No connection to database");
        return -1;
    }
    return modify_config_priorities_ex(conn, asset_name, configuration_id_list);
}

/**
 * @function insert_config_ex Insert a new configuration for an asset
 * @param conn The connection to the database
 * @param asset_name The asset name to add  new configuration
 * @param is_working Value of is_working attribute
 * @param is_enabled Value of is_enabled attribute
 * @param key_value_asset_list The list of key values to add in the asset configuration attribute table
 * @return {integer} configuration id if no error else < 0
 */
int insert_config_ex (tntdb::Connection conn, std::string asset_name, int config_type, bool is_working, bool is_enabled, std::map<std::string, std::string>& key_value_asset_list)
{
    int64_t asset_id = get_asset_id(conn, asset_name);
    if (asset_id < 0) {
        log_error("Element %s not found", asset_name.c_str());
        return -2;
    }
    try {
        // Get max priority
        tntdb::Statement st = conn.prepareCached(
            " SELECT MAX(priority)"
            " FROM"
            "   t_bios_nut_configuration"
            " WHERE id_asset_element = :asset_id"
        );
        tntdb::Row row = st.set("asset_id", asset_id).selectRow();
        int max_priority = -1;
        row[0].get(max_priority);

        // Insert new config
        st = conn.prepareCached(
            " INSERT INTO t_bios_nut_configuration"
            " (id_nut_configuration_type, id_asset_element, priority, is_enabled, is_working)"
            " VALUES "
            " (:config_type, :asset_id, :priority, :is_enabled, :is_working)"
        );
        st.set("config_type", config_type).
            set("asset_id", asset_id).
            set("priority", max_priority + 1).
            set("is_enabled", is_enabled).
            set("is_working", is_working).
            execute();
        int config_id = conn.lastInsertId();
        if (config_id < 0) {
            log_error("No id when added new configuration for %s", asset_name.c_str());
            return -3;
        }

        // Insert the list of key values in the asset configuration attribute table
        std::ostringstream s;
        s << " INSERT IGNORE INTO t_bios_nut_configuration_attribute"
          << " (id_nut_configuration, keytag, value)"
          << " VALUES";
        int nb;
        for (nb = 0; nb < key_value_asset_list.size() - 1; nb ++) {
            s << " (:config_id, :key_" << nb << ", :value_" << nb << "),";
        }
        s << " (:config_id, :key_" << nb << ", :value_" << nb << ")";
        st = conn.prepareCached(s.str());
        st = st.set("config_id", config_id);
        nb = 0;
        for (auto it = key_value_asset_list.begin(); it != key_value_asset_list.end(); ++it) {
            s.str("");
            s << "key_" << nb;
            st = st.set(s.str(), it->first);
            s.str("");
            s << "value_" << nb;
            st = st.set(s.str(), it->second);
            nb ++;
        }
        st.execute();
        return config_id;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -4;
    }
}

/**
 * @function insert_config Insert a new configuration for an asset
 * @param asset_name The asset name to add  new configuration
 * @param is_working Value of is_working attribute
 * @param is_enabled Value of is_enabled attribute
 * @param key_value_asset_list The list of key values to add in the asset configuration attribute table
 * @return {integer} configuration id if no error else < 0
 */
int insert_config (std::string asset_name, int config_type, bool is_working, bool is_enabled, std::map<std::string, std::string>& key_value_asset_list)
{
    tntdb::Connection conn;
    try {
        conn = tntdb::connect(DBConn::url);
    }
    catch(...)
    {
        log_error("No connection to database");
        return -1;
    }
    return insert_config_ex(conn, asset_name, config_type, is_working, is_enabled, key_value_asset_list);
}

/**
 * @function remove_config_ex Remove a configuration from database
 * @param conn The connection to the database
 * @param config_id The configuration id to remove
 * @return {integer} 0 if no error else < 0
 */
int remove_config_ex (tntdb::Connection conn, int config_id)
{
    try {
        // Remove configuration in t_bios_nut_configuration_secw_document table
        tntdb::Statement st = conn.prepareCached(
            " DELETE"
            " FROM"
            "   t_bios_nut_configuration_secw_document"
            " WHERE"
            "   id_nut_configuration = :config_id"
        );
        st.set("config_id", config_id).execute();

        // Then remove configuration in t_bios_nut_configuration_attribute table
        st = conn.prepareCached(
            " DELETE"
            " FROM"
            "   t_bios_nut_configuration_attribute"
            " WHERE"
            "   id_nut_configuration = :config_id"
        );
        st.set("config_id", config_id).execute();

        // Finally, remove configuration in t_bios_nut_configuration table
        st = conn.prepareCached(
            " DELETE"
            " FROM"
            "   t_bios_nut_configuration"
            " WHERE"
            "   id_nut_configuration = :config_id"
        );
        st.set("config_id", config_id).execute();
        return 0;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -2;
    }
}

/**
 * @function remove_config Remove a configuration from database
 * @param config_id The configuration id to remove
 * @return {integer} 0 if no error else < 0
 */
int remove_config (int config_id)
{
    tntdb::Connection conn;
    try {
        conn = tntdb::connect(DBConn::url);
    }
    catch(...)
    {
        log_error("No connection to database");
        return -1;
    }
    return remove_config_ex(conn, config_id);
}

/**
 * @function get_configuration_from_type Get specific configuration information for each configuration type
 * @param conn The connection to the database
 * @param config_type The configuration type
 * @param [out] config_info The returned configuration information
 * @return {integer} 0 if no error else < 0
 */
int get_configuration_from_type_ex (tntdb::Connection conn, /*int config_type,*/ ConfigurationInfoList& config_info_list)
{
    try {
        // Get all configuration type
        tntdb::Statement st = conn.prepareCached(
            " SELECT id_nut_configuration_type, configuration_name, driver, port"
            " FROM"
            "   t_bios_nut_configuration_type"
        );
        tntdb::Result result = st.select();
        for (auto &row: result) {
            int config_type;
            row["id_nut_configuration_type"].get(config_type);
            std::string config_name;
            row["configuration_name"].get(config_name);
            std::string driver;
            row["driver"].get(driver);
            std::string port;
            row["port"].get(port);

            // Get all default key values according configuration type value
            std::map<std::string, std::string> default_values_list;
            tntdb::Statement st1 = conn.prepareCached(
                " SELECT keytag, value"
                " FROM"
                "   t_bios_nut_configuration_default_attribute"
                " WHERE id_nut_configuration_type = :config_type"
            );
            tntdb::Result result1 = st1.set("config_type", config_type).select();
            for (auto &row1: result1) {
                std::string key;
                row1["keytag"].get(key);
                std::string value;
                row1["value"].get(value);
                default_values_list.insert(std::make_pair(key, value));
            }

            // Get all document type according configuration type value
            std::vector<std::string> document_type_list;
            tntdb::Statement st2 = conn.prepareCached(
                " SELECT id_secw_document_type"
                " FROM"
                "   t_bios_nut_configuration_type_secw_document_type_requirements"
                " WHERE id_nut_configuration_type = :config_type"
            );
            tntdb::Result result2 = st2.set("config_type", config_type).select();
            for (auto &row2: result2) {
                std::string document_type;
                row2["id_secw_document_type"].get(document_type);
                document_type_list.push_back(document_type);
            }
            ConfigurationInfo config_info = std::make_tuple(config_type, config_name, driver, port, default_values_list, document_type_list);
            config_info_list.push_back(config_info);
        }
        return 0;
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -2;
    }
}

/**
 * @function get_configuration_from_type Get specific configuration information for each configuration type
 * @param config_type The configuration type
 * @param [out] config_info The returned configuration information
 * @return {integer} 0 if no error else < 0
 */
int get_configuration_from_type (/*int config_type, */ConfigurationInfoList& config_info_list)
{
    tntdb::Connection conn;
    try {
        conn = tntdb::connect(DBConn::url);
    }
    catch(...)
    {
        log_error("No connection to database");
        return -1;
    }
    return get_configuration_from_type_ex(conn, /* config_type, */ config_info_list);
}

/**
 * @function get_config_type_from_info Get configuration type from specific configuration information
  * @param config_info The specific configuration information
  * @return {integer} Return configuration type
 */
int get_config_type_from_info(ConfigurationInfo& config_info) { return std::get<0>(config_info); };

/**
 * @function get_config_name_from_info Get configuration name from specific configuration information
  * @param config_info The specific configuration information
  * @return {string} Return configuration name
 */
std::string get_config_name_from_info(ConfigurationInfo& config_info) { return std::get<1>(config_info); };

/**
 * @function get_config_driver_from_info Get configuration driver from specific configuration information
  * @param config_info The specific configuration information
  * @return {string} Return configuration name
 */
std::string get_config_driver_from_info(ConfigurationInfo& config_info) { return std::get<2>(config_info); };

/**
 * @function get_config_port_from_info Get configuration port from specific configuration information
  * @param config_info The specific configuration information
  * @return {string} Return configuration port
 */
std::string get_config_port_from_info(ConfigurationInfo& config_info) { return std::get<3>(config_info); };

/**
 * @function get_config_default_key_value_list_from_info Get configuration default key value list from specific configuration information
  * @param config_info The specific configuration information
  * @return {map} Return configuration default key value list
 */
std::map<std::string, std::string> get_config_default_key_value_list_from_info(ConfigurationInfo& config_info) { return std::get<4>(config_info); };

/**
 * @function get_config_document_type_list_from_info Get configuration document type list from specific configuration information
  * @param config_info The specific configuration information
  * @return {list} Return configuration document type list
 */
std::vector<std::string> get_config_document_type_list_from_info(ConfigurationInfo& config_info) { return std::get<5>(config_info); };

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

// FIXME: No necessary rights for this diectory
//std::string run_working_path_test("/var/run/fty_common_db_discovery");
std::string run_working_path_test("/home/admin/fty_common_db_discovery");

int test_start_database (std::string test_working_dir)
{
    int mysql_port = 30001;

    std::stringstream buffer;
    // Create selftest-rw if not exist
    buffer << "mkdir " << SELFTEST_DIR_RW;
    std::string command = buffer.str();
    system(command.c_str());
    // Create working path for test
    buffer.str("");
    buffer << "mkdir " << run_working_path_test;
    command = buffer.str();
    system(command.c_str());
    // Create shell script to execute
    buffer.str("");
    buffer << test_working_dir << "/" << "start_sql_server.sh";
    std::string file_path = buffer.str();
    std::ofstream file;
    file.open(file_path);
    file << "#!/bin/bash\n";
    file << "TEST_PATH=" << test_working_dir << "\n";
    file << "mkdir $TEST_PATH\n";
    file << "mkdir $TEST_PATH/db\n";
    file << "mysql_install_db --datadir=$TEST_PATH/db\n";
    file << "mkfifo " << run_working_path_test << "/mysqld.sock\n";
    file << "/usr/sbin/mysqld --no-defaults --pid-file=" << run_working_path_test << "/mysqld.pid";
    file << " --datadir=$TEST_PATH/db --socket=" << run_working_path_test << "/mysqld.sock";
    file << " --port " << mysql_port << " &\n";
    file << "sleep 5\n";
    file << "mysql -u root -S " << run_working_path_test << "/mysqld.sock < /usr/share/bios/sql/mysql/initdb.sql\n";
    file << "for i in $(ls /usr/share/bios/sql/mysql/0*.sql | sort); do mysql -u root -S " << run_working_path_test << "/mysqld.sock < $i; done\n";
    file.close();

    // Change the right of the shell script for execution
    int ret = chmod(file_path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);

    // Execute the shell script
    system(file_path.c_str());

    // Remove the shell script
    remove(file_path.c_str());
}

void test_stop_database (std::string test_working_dir)
{
    // Create shell script to execute
    std::stringstream buffer;
    buffer << test_working_dir << "/" << "stop_sql_server.sh";
    std::string file_path = buffer.str();
    std::ofstream file;
    file.open(file_path);
    file << "#!/bin/bash\n";
    file << "read -r PID < \"" << run_working_path_test << "/mysqld.pid\"\n";
    file << "echo $PID\n";
    file << "kill $PID\n";
    file << "rm -rf " << test_working_dir << "/db\n";
    file << "rm -rf " << run_working_path_test << "\n";
    file.close();

    // Change the right of the shell script for execution
    int ret = chmod(file_path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);

    // Execute the shell script
    system(file_path.c_str());

    // Remove the shell script
    remove(file_path.c_str());
}

int test_op_table (tntdb::Connection& conn, std::string request_table)
{
    try {
        tntdb::Statement st = conn.prepareCached(request_table);
        uint64_t res = st.execute();
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
    return 0;
}

int test_get_priorities_base (tntdb::Connection& conn, int asset_id, std::vector<std::pair< std::string, std::string>>& configuration_id_list)
{
    try {
        tntdb::Statement st = conn.prepareCached(
            " SELECT id_nut_configuration, priority"
            " FROM"
            "   t_bios_nut_configuration"
            " WHERE id_asset_element = :asset_id"
            " ORDER BY priority ASC"
        );
        tntdb::Result result = st.set("asset_id", asset_id).select();
        for (auto &row: result) {
            std::string config_id;
            std::string priority;
            row["id_nut_configuration"].get(config_id);
            row["priority"].get(priority);
            configuration_id_list.push_back(std::make_pair(config_id, priority));
        }
    }
    catch (const std::exception &e) {
        LOG_END_ABNORMAL(e);
        return -1;
    }
    return 0;
}

// FIXME: Not used
void test_del_data_database (tntdb::Connection& conn)
{
    assert(test_op_table(conn, std::string("DELETE FROM t_bios_nut_configuration_default_attribute")) == 0);
    assert(test_op_table(conn, std::string("DELETE FROM t_bios_nut_configuration_attribute")) == 0);
    assert(test_op_table(conn, std::string("DELETE FROM t_bios_nut_configuration_type_secw_document_type_requirements")) == 0);
    assert(test_op_table(conn, std::string("DELETE FROM t_bios_nut_configuration_secw_document")) == 0);
    assert(test_op_table(conn, std::string("DELETE FROM t_bios_nut_configuration")) == 0);
    assert(test_op_table(conn, std::string("DELETE FROM t_bios_nut_configuration_type")) == 0);
    assert(test_op_table(conn, std::string("DELETE FROM t_bios_secw_document")) == 0);
    assert(test_op_table(conn, std::string("DELETE FROM t_bios_secw_document_type")) == 0);
    assert(test_op_table(conn, std::string("DELETE FROM t_bios_asset_element WHERE id_asset_element <> 1")) == 0);
}

void fty_common_db_discovery_test (bool verbose)
{
    printf (" * fty_common_db_discovery: ");

    std::map<std::string, std::vector<std::map<std::string, std::string>>> test_results = {
        {
            "ups-1",
            {
                {
                    { "mibs", "eaton_ups" },
                    { "pollfreq", "21" },
                    { "snmp_retries", "201" },
                    { "snmp_version", "v3" },
                    { "synchronous", "yes" }
                },
                {
                    { "mibs", "eaton_ups" },
                    { "pollfreq", "11" },
                    { "snmp_retries", "101" },
                    { "snmp_version", "v1" },
                    { "synchronous", "yes" }
                }
            }
        },
        {
            "ups-2",
            {
                {
                    { "mibs", "eaton_ups" },
                    { "pollfreq", "51" },
                    { "snmp_retries", "501" },
                    { "snmp_version", "v3" },
                    { "synchronous", "yes" }
                }
            }
        },
        {
            "ups-3",
            {
                {
                    { "pollfreq", "91"},
                    { "protocol", "{asset.protocol.http:http}" },
                    { "snmp_retries", "901" },
                    { "synchronous", "no" }
                }
            }
        }
    };

    // Get current directory
    char current_working_dir[FILENAME_MAX];
    getcwd(current_working_dir, FILENAME_MAX);
    // Set working test directory
    std::stringstream buffer;
    buffer << current_working_dir << "/" << SELFTEST_DIR_RW;
    std::string test_working_dir = buffer.str();

    // FIXME
    // Stop previous instance of database if test failed
    test_stop_database(test_working_dir);

    // Create and start database for test
    test_start_database(test_working_dir);

    tntdb::Connection conn;
    try {
        buffer.str("");
        buffer << "mysql:db=box_utf8;user=root;unix_socket=" << run_working_path_test << "/mysqld.sock";
        std::string url = buffer.str();
        conn = tntdb::connect(url);
    }
    catch(...)
    {
        log_fatal("error connection database");
        assert(0);
    }

    // Remove tables data if previous tests failed
    //test_del_data_database(conn);

    std::string t_asset_name[] = { "ups-1", "ups-2", "ups-3" };
    int nb_assets = sizeof(t_asset_name) / sizeof(char *);
    int64_t t_asset_id[nb_assets];

    uint16_t element_type_id = 6;  // ups
    uint32_t parent_id = 1;  // rack
    const char *status = "active";
    uint16_t priority = 5;
    uint16_t subtype_id = 1;
    const char *asset_tag = NULL;
    bool update = true;
    db_reply_t res;
    // Update assets in database
    for (int i = 0; i < nb_assets; i ++) {
        res = DBAssetsInsert::insert_into_asset_element(
            conn,
            t_asset_name[i].c_str(),
            element_type_id,
            parent_id,
            status,
            priority,
            subtype_id,
            asset_tag,
            update
        );
        assert(res.status == 1);
        t_asset_id[i] = DBAssetsDiscovery::get_asset_id(conn, t_asset_name[i]);
    }

    // Data for table t_bios_secw_document_type
    assert(test_op_table(conn, std::string(
        " INSERT IGNORE INTO t_bios_secw_document_type"
        " (id_secw_document_type)"
        " VALUES"
        " ('Snmpv1'),"
        " ('Snmpv3'),"
        " ('UserAndPassword'),"
        " ('ExternalCertificate'),"
        " ('InternalCertificate')")) == 0
    );

    // Data for table t_bios_secw_document
    assert(test_op_table(conn, std::string(
        " INSERT IGNORE INTO t_bios_secw_document"
        " (id_secw_document, id_secw_document_type)"
        " VALUES"
        " (UUID_TO_BIN('11111111-1111-1111-1111-000000000001'), 'Snmpv1'),"
        " (UUID_TO_BIN('11111111-1111-1111-1111-000000000002'), 'Snmpv1'),"
        " (UUID_TO_BIN('22222222-2222-2222-2222-000000000001'), 'Snmpv3'),"
        " (UUID_TO_BIN('22222222-2222-2222-2222-000000000002'), 'Snmpv3'),"
        " (UUID_TO_BIN('33333333-3333-3333-3333-000000000001'), 'UserAndPassword'),"
        " (UUID_TO_BIN('33333333-3333-3333-3333-000000000002'), 'UserAndPassword')")) == 0
    );

    // Data for table t_bios_nut_configuration_type
    assert(test_op_table(conn, std::string(
        " INSERT IGNORE INTO t_bios_nut_configuration_type"
        " (id_nut_configuration_type, configuration_name, driver, port)"
        " VALUES"
        " (1, 'Driver snmpv1 ups', 'snmp-ups', '{asset.ip.1}:{asset.port.snmpv1:161}'),"
        " (2, 'Driver snmpv3 ups', 'snmp-ups', '{asset.ip.1}:{asset.port.snmpv3:161}'),"
        " (3, 'Driver xmlv3 http ups', 'xmlv3-ups', 'http://{asset.ip.1}:{asset.port.http:80}'),"
        " (4, 'Driver xmlv3 https ups', 'xmlv3-ups', 'https://{asset.ip.1}:{asset.port.http:443}'),"
        " (5, 'Driver xmlv4 http ups', 'xmlv4-ups', 'http://{asset.ip.1}:{asset.port.http:80}'),"
        " (6, 'Driver xmlv4 https ups', 'xmlv4-ups', 'https://{asset.ip.1}:{asset.port.http:443}'),"
        " (7, 'Driver mqtt https ups', 'mqtt-ups', 'https://{asset.ip.1}:{asset.port.http:443}'),"
        " (8, 'Driver mqtt ups', 'mqtt-ups', 'https://{asset.ip.1}:{asset.port.http:443}'),"
        " (9, 'Driver mqtts ups', 'mqtt-ups', 'https://{asset.ip.1}:{asset.port.http:443}')")) == 0
    );

    // Data for table t_bios_nut_configuration
    std::stringstream ss;
    ss << " INSERT IGNORE INTO t_bios_nut_configuration"
       << " (id_nut_configuration, id_nut_configuration_type, id_asset_element, priority, is_enabled, is_working)"
       << " VALUES"
       << " (1, 1, " << t_asset_id[0] << ", 2, TRUE, TRUE),"  // 0: 1st Active
       << " (2, 2, " << t_asset_id[0] << ", 1, TRUE, TRUE),"  // 0: 2nd Active (it is the most priority)
       << " (3, 3, " << t_asset_id[0] << ", 0, FALSE, TRUE),"
       << " (4, 1, " << t_asset_id[1] << ", 0, FALSE, TRUE),"
       << " (5, 2, " << t_asset_id[1] << ", 1, TRUE, TRUE),"  // 1: 1st Active
       << " (6, 3, " << t_asset_id[1] << ", 2, FALSE, TRUE),"
       << " (7, 1, " << t_asset_id[2] << ", 0, FALSE, TRUE),"
       << " (8, 2, " << t_asset_id[2] << ", 1, FALSE, TRUE),"
       << " (9, 3, " << t_asset_id[2] << ", 2, TRUE, TRUE)";  // 1: 1st Active
    assert(test_op_table(conn, ss.str()) == 0);

    // Data for table t_bios_nut_configuration_secw_document
    assert(test_op_table(conn, std::string(
        " INSERT IGNORE INTO t_bios_nut_configuration_secw_document"
        " (id_nut_configuration, id_secw_document)"
        " VALUES"
        " (1, UUID_TO_BIN('11111111-1111-1111-1111-000000000001')),"
        " (1, UUID_TO_BIN('11111111-1111-1111-1111-000000000002')),"
        " (2, UUID_TO_BIN('22222222-2222-2222-2222-000000000001')),"
        " (5, UUID_TO_BIN('22222222-2222-2222-2222-000000000002')),"
        " (9, UUID_TO_BIN('33333333-3333-3333-3333-000000000001')),"
        " (9, UUID_TO_BIN('33333333-3333-3333-3333-000000000002'))")) == 0
    );

    // Data for table t_bios_nut_configuration_type_secw_document_type_requirements
    assert(test_op_table(conn, std::string(
        " INSERT IGNORE INTO t_bios_nut_configuration_type_secw_document_type_requirements"
        " (id_nut_configuration_type, id_secw_document_type)"
        " VALUES"
        " (1, 'Snmpv1'),"
        " (2, 'Snmpv1'),"
        " (2, 'Snmpv3'),"
        " (3, 'UserAndPassword'),"
        " (4, 'UserAndPassword'),"
        " (5, 'UserAndPassword'),"
        " (6, 'UserAndPassword')")) == 0
    );

    // Data for table t_bios_nut_configuration_attribute
    assert(test_op_table(conn, std::string(
        " INSERT IGNORE INTO t_bios_nut_configuration_attribute"
        " (id_nut_configuration, keytag, value)"
        " VALUES"
        " (1, 'snmp_retries', '101'),"
        " (1, 'pollfreq', '11'),"
        " (1, 'synchronous', 'yes'),"
        " (2, 'snmp_retries', '201'),"
        " (2, 'pollfreq', '21'),"
        " (2, 'synchronous', 'yes'),"
        " (5, 'snmp_retries', '501'),"
        " (5, 'pollfreq', '51'),"
        " (5, 'synchronous', 'yes'),"
        " (9, 'snmp_retries', '901'),"
        " (9, 'pollfreq', '91'),"
        " (9, 'synchronous', 'no')")) == 0
    );

    // Data for table t_bios_nut_configuration_default_attribute
    assert(test_op_table(conn, std::string(
        " INSERT IGNORE INTO t_bios_nut_configuration_default_attribute"
        " (id_nut_configuration_type, keytag, value)"
        " VALUES"
        " (1, 'mibs', 'eaton_ups'),"
        " (1, 'pollfreq', '10'),"
        " (1, 'snmp_retries', '100'),"
        " (2, 'mibs', 'eaton_ups'),"
        " (2, 'pollfreq', '20'),"
        " (1, 'snmp_retries', '200'),"
        " (3, 'protocol', '{asset.protocol.http:http}'),"
        " (3, 'pollfreq', '30'),"
        " (3, 'snmp_retries', '300'),"
        " (1, 'snmp_version', 'v1'),"
        " (2, 'snmp_version', 'v3')")) == 0
    );

    int asset_id = -1;
    int config_id = -1;
    int config_type = -1;

    int i = 0;
    // Test for each asset
    for (int i = 0; i < nb_assets; i ++) {
        asset_id = t_asset_id[i];
        std::cout << "\n<<<<<<<<<<<<<<<<<<< Test with asset " << t_asset_name[i] << "/" << asset_id << ":" << std::endl;

        // Test get_candidate_config function
        {
            nutcommon::DeviceConfiguration device_config_list;
            std::cout << "\nTest get_candidate_config for " << t_asset_name[i] << ":" << std::endl;
            std::string device_config_id;
            int res = DBAssetsDiscovery::get_candidate_config_ex (conn, t_asset_name[i], device_config_id, device_config_list);
            assert(res == 0);
            std::map<std::string, std::string> key_value_res = test_results[t_asset_name[i]].at(0);
            assert(key_value_res.size() == device_config_list.size());
            std::map<std::string, std::string>::iterator itr;
            for (itr = device_config_list.begin(); itr != device_config_list.end(); ++itr) {
                std::cout << "[" << itr->first << "] = " << itr->second << std::endl;
                assert(key_value_res.at(itr->first) == itr->second);
            }
        }

        // Test get_candidate_config_list function
        {
            DBAssetsDiscovery::DeviceConfigurationIdList device_config_id_list;
            std::cout << "\nTest get_candidate_configs for " << t_asset_name[i] << ":" << std::endl;
            int res = DBAssetsDiscovery::get_config_list_ex (conn, REQUEST_WHERE_CANDIDATE_CONFIG, t_asset_name[i], device_config_id_list);
            assert(res == 0);
            assert(test_results[t_asset_name[i]].size() == device_config_id_list.size());
            int nb_config = 0;
            auto it_config = device_config_id_list.begin();
            auto it_config_res = test_results[t_asset_name[i]].begin();
            while (it_config != device_config_id_list.end() && it_config_res != test_results[t_asset_name[i]].end()) {
                if (nb_config ++ != 0) std::cout << "<<<<<<<<<<<<" <<  std::endl;
                std::map<std::string, std::string> key_value_res = *it_config_res;
                assert(key_value_res.size() == (*it_config).second.size());
                auto it = (*it_config).second.begin();
                while (it != (*it_config).second.end()) {
                    std::cout << "[" << it->first << "] = " << it->second << std::endl;
                    assert(key_value_res.at(it->first) == it->second);
                    it ++;
                }
                it_config ++;
                it_config_res ++;
            }
        }

        // Test get_all_config_list function
        {
            DBAssetsDiscovery::DeviceConfigurationIdList device_config_id_list;
            std::cout << "\nTest get_candidate_configs for " << t_asset_name[i] << ":" << std::endl;
            int res = DBAssetsDiscovery::get_config_list_ex (conn, REQUEST_WHERE_ALL_CONFIG, t_asset_name[i], device_config_id_list);
            assert(res == 0);
            std::cout << "size=" << device_config_id_list.size() << std::endl;
            assert(device_config_id_list.size() == 3);
        }
    }

    // Test get/set functions for configuration working value
    {
        bool value, initial_value;
        std::string config_id = "1";
        // Get initial value
        assert(DBAssetsDiscovery::get_config_working_ex(conn, config_id, initial_value) == 0);
        // Change current value
        assert(DBAssetsDiscovery::set_config_working_ex(conn, config_id, !initial_value) == 0);
        // Retry to change current value
        assert(DBAssetsDiscovery::set_config_working_ex(conn, config_id, !initial_value) == 0);
        // Get current value
        assert(DBAssetsDiscovery::get_config_working_ex(conn, config_id, value) == 0);
        assert(initial_value != value);
        // Restore initial value
        assert(DBAssetsDiscovery::set_config_working_ex(conn, config_id, initial_value) == 0);
        // Get current value
        assert(DBAssetsDiscovery::get_config_working_ex(conn, config_id, value) == 0);
        assert(initial_value == value);
    }

    // Test modify_config_priorities function
    {
        std::string asset_name = "ups-1";
        std::vector<std::pair< std::string, std::string>> config_priority_list;
        std::vector<std::string> init_config_id_list;
        std::vector<std::string> config_id_list;
        int asset_id = DBAssetsDiscovery::get_asset_id(conn, asset_name);
        assert(test_get_priorities_base(conn, asset_id, config_priority_list) == 0);
        // Save initial priority order
        for (auto it = config_priority_list.begin(); it != config_priority_list.end(); it ++) {
            init_config_id_list.push_back(it->first);
        }
        // Inverse priority order
        for (auto rit = config_priority_list.rbegin(); rit != config_priority_list.rend(); ++ rit) {
            config_id_list.push_back(rit->first);
        }
        // Apply priority order changing
        assert(DBAssetsDiscovery::modify_config_priorities_ex(conn, asset_name, config_id_list) == 0);
        // Read and check result
        config_priority_list.erase(config_priority_list.begin(), config_priority_list.end());
        assert(test_get_priorities_base(conn, asset_id, config_priority_list) == 0);
        int num_priority = 0;
        auto it_config_priority_list = config_priority_list.begin();
        auto it_config_id_list = config_id_list.begin();
        while (it_config_priority_list != config_priority_list.end() && it_config_id_list != config_id_list.end()) {
            assert((*it_config_priority_list).first == *it_config_id_list);
            assert(num_priority == std::stoi(it_config_priority_list->second));
            num_priority ++;
            it_config_priority_list ++;
            it_config_id_list ++;
        }
        // Restore previous priority order
        assert(DBAssetsDiscovery::modify_config_priorities_ex(conn, asset_name, init_config_id_list) == 0);
        // Read and check result
        config_priority_list.erase(config_priority_list.begin(), config_priority_list.end());
        assert(test_get_priorities_base(conn, asset_id, config_priority_list) == 0);
        num_priority = 0;
        it_config_priority_list = config_priority_list.begin();
        auto it_init_config_id_list = init_config_id_list.begin();
        while (it_config_priority_list != config_priority_list.end() && it_init_config_id_list != init_config_id_list.end()) {
            assert((*it_config_priority_list).first == *it_init_config_id_list);
            assert(num_priority == std::stoi(it_config_priority_list->second));
            num_priority ++;
            it_config_priority_list ++;
            it_init_config_id_list ++;
        }
    }

    // Test insert_config function
    {
        std::map<std::string, std::string> key_value_asset_list = {{ "Key1", "Val1"}, { "Key2", "Val2"}, { "Key3", "Val3"}};
        int config_type = 1;
        // Insert new config
        int config_id = DBAssetsDiscovery::insert_config_ex(conn, "ups-1", config_type, true, true, key_value_asset_list);
        assert(config_id >= 0);
        // Remove inserted config
        assert(DBAssetsDiscovery::remove_config_ex(conn, config_id) == 0);
    }

    // Test get_configuration_from_type function
    {
        DBAssetsDiscovery::ConfigurationInfoList config_info_list;
        assert(DBAssetsDiscovery::get_configuration_from_type_ex(conn, /*config_type,*/ config_info_list) == 0);
        auto it_config_info_list = config_info_list.begin();
        while (it_config_info_list != config_info_list.end() && it_config_info_list != config_info_list.end()) {
            std::cout << "--------------" << std::endl;
            std::cout << "type=" << DBAssetsDiscovery::get_config_type_from_info(*it_config_info_list) << std::endl;
            std::cout << "name=" << DBAssetsDiscovery::get_config_name_from_info(*it_config_info_list) << std::endl;
            std::cout << "driver=" << DBAssetsDiscovery::get_config_driver_from_info(*it_config_info_list) << std::endl;
            std::cout << "port=" << DBAssetsDiscovery::get_config_port_from_info(*it_config_info_list) << std::endl;
            std::map<std::string, std::string> key_value_list = DBAssetsDiscovery::get_config_default_key_value_list_from_info(*it_config_info_list);
            auto it_key_value_list = key_value_list.begin();
            while (it_key_value_list != key_value_list.end() && it_key_value_list != key_value_list.end()) {
                std::cout << "  " << it_key_value_list->first << "=" << it_key_value_list->second << std::endl;
                it_key_value_list ++;
            }
            std::vector<std::string> document_type_list = DBAssetsDiscovery::get_config_document_type_list_from_info(*it_config_info_list);
            auto it_document_type_list = document_type_list.begin();
            while (it_document_type_list != document_type_list.end() && it_document_type_list != document_type_list.end()) {
                std::cout << *it_document_type_list << std::endl;
                it_document_type_list ++;
            }
            it_config_info_list ++;
        }
    }

    // Remove data previously added in database
    //test_del_data_database(conn);

    // Stop and remove database
    test_stop_database(test_working_dir);

    printf ("\nEnd tests \n");
}


