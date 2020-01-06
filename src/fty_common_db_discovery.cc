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
#include <fty_common_nut_credentials.h>

#include <assert.h>

namespace DBAssetsDiscovery {

/**
 * @function get_asset_id get asset id from asset name
 * @param conn The connection to the database
 * @param asset_name The asset name to search
 * @return {integer} the asset id found if success else < 0
 */
static uint64_t get_asset_id (tntdb::Connection& conn, const std::string& asset_name)
{
    uint64_t id = 0;
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

#if 0
/**
 * @function get_database_config Request in database first candidate configuration of an asset
 * @param conn the connection to the database
 * @param request The request to send to the database
 * @param asset_id The asset id to get configuration
 * @param config_id [out] The return configuration id
 * @param device_config [out] The return configuration of the asset
 * @return {integer} 0 if no error else < 0
 */
static int request_database_config (tntdb::Connection& conn, const std::string& request, const int64_t asset_id, std::string &config_id, nutcommon::DeviceConfiguration& device_config)
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
 * @function get_candidate_config Get first candidate configuration of an asset
 * @param conn The connection to the database
 * @param asset_name The asset name to get configuration
 * @param config_id [out] The return configuration id
 * @param device_config [out] The return configuration of the asset
 * @return {integer} 0 if no error else < 0
 */
int get_candidate_config (tntdb::Connection& conn, const std::string& asset_name, std::string &config_id, nutcommon::DeviceConfiguration& device_config)
{
   const  int64_t asset_id = get_asset_id(conn, asset_name);
    if (asset_id < 0) {
        log_error("Element %s not found", asset_name.c_str());
        return -1;
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
            return -2;
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
            return -3;
        }

        if (default_config_id != asset_config_id) {
            log_error("Default config id different of asset config id");
            return -4;
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
        return -5;
    }
}
#endif

/**
 * @function request_database_config_list Request in database all candidate configurations of an asset
 * @param conn The connection to the database
 * @param request The request to send to the database
 * @param asset_id The asset id to get configuration
 * @return The return configuration list of the asset
 */
static DeviceConfigurationInfos request_database_config_list (tntdb::Connection& conn, const std::string& request, const uint64_t asset_id)
{
    DeviceConfigurationInfos device_config_id_list;

    tntdb::Statement st = conn.prepareCached(request);
    tntdb::Result res = st.set("asset_id", asset_id).select();
    size_t config_id_in_progress = -1;
    nutcommon::DeviceConfiguration config;
    for (auto &row: res) {
        size_t config_id = -1;
        row["id_nut_configuration"].get(config_id);
        if (config_id_in_progress == -1) {
            config_id_in_progress = config_id;
        }
        if (config_id_in_progress != config_id) {
            if (!config.empty()) {
                DeviceConfigurationInfo config_info;
                config_info.id = config_id_in_progress;
                config_info.attributes = config;
                device_config_id_list.emplace_back(config_info);
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
        DeviceConfigurationInfo config_info;
        config_info.id = config_id_in_progress;
        config_info.attributes = config;
        device_config_id_list.emplace_back(config_info);
    }

    return device_config_id_list;
}

/**
 * @function get_config_list Get configurations of an asset depending of where condition request
 * @param conn The connection to the database
 * @param request_where The where condition of the request to send to the database
 * @param asset_name The asset name to get configuration
 * @return The return configuration list of the asset
 */
static DeviceConfigurationInfos get_config_list (tntdb::Connection& conn, const std::string& request_where, const std::string& asset_name)
{
    DeviceConfigurationInfos device_config_id_list;

    const uint64_t asset_id = get_asset_id(conn, asset_name);

    // Get first default configurations
    std::string request = " SELECT config.id_nut_configuration as id_nut_configuration, conf_def_attr.keytag as keytag, conf_def_attr.value as value, config.priority as priority"
        " FROM t_bios_nut_configuration config"
        " INNER JOIN t_bios_nut_configuration_default_attribute conf_def_attr"
        " ON conf_def_attr.id_nut_configuration_type = config.id_nut_configuration_type";
    request += request_where;
    request += R"xxx( UNION SELECT config.id_nut_configuration as id_nut_configuration, "driver" as keytag, confType.driver as value, config.priority as priority FROM t_bios_nut_configuration_type confType JOIN t_bios_nut_configuration config ON confType.id_nut_configuration_type = config.id_nut_configuration_type)xxx" + request_where;
    request += R"xxx( UNION SELECT config.id_nut_configuration as id_nut_configuration, "port" as keytag, confType.port as value, config.priority as priority FROM t_bios_nut_configuration_type confType JOIN t_bios_nut_configuration config ON confType.id_nut_configuration_type = config.id_nut_configuration_type)xxx" + request_where;
    request += " ORDER BY priority ASC, id_nut_configuration";

    DeviceConfigurationInfos default_config_id_list = request_database_config_list(conn, request, asset_id);

    // Then get asset configurations
    request = " SELECT config.id_nut_configuration, conf_attr.keytag, conf_attr.value"
        " FROM t_bios_nut_configuration config"
        " INNER JOIN t_bios_nut_configuration_attribute conf_attr"
        " ON conf_attr.id_nut_configuration = config.id_nut_configuration";
    request += request_where;
    request += " ORDER BY config.priority ASC, config.id_nut_configuration";

    DeviceConfigurationInfos asset_config_id_list = request_database_config_list(conn, request, asset_id);

    // Save first part of result
    auto it_default_config_id_list = default_config_id_list.begin();
    while (it_default_config_id_list != default_config_id_list.end()) {
        DeviceConfigurationInfo config_info;
        config_info.id = (*it_default_config_id_list).id;
        config_info.attributes.insert((*it_default_config_id_list).attributes.begin(), (*it_default_config_id_list).attributes.end());
        device_config_id_list.emplace_back(config_info);
        it_default_config_id_list ++;
    }

    // Merge asset config to default config:
    auto it_asset_config_id_list = asset_config_id_list.begin();
    while (it_asset_config_id_list != asset_config_id_list.end()) {
        size_t config_id = (*it_asset_config_id_list).id;
        auto it_config_id_list = std::find_if(device_config_id_list.begin(), device_config_id_list.end(),
            [&config_id](const DeviceConfigurationInfo& val){ return val.id == config_id; });
        // If config id not found
        if (it_config_id_list == device_config_id_list.end()) {
            // - insert new elements from asset config
            DeviceConfigurationInfo config_info;
            config_info.id = config_id;
            config_info.attributes.insert((*it_asset_config_id_list).attributes.begin(), (*it_asset_config_id_list).attributes.end());
            device_config_id_list.emplace_back(config_info);
        }
        // else config id has been found
        else {
            // - first add new elements from asset config if not present in default config
            (*it_config_id_list).attributes.insert((*it_asset_config_id_list).attributes.begin(), (*it_asset_config_id_list).attributes.end());
            // - then update default element value with asset config value if their keys are identical
            for(auto& it : (*it_asset_config_id_list).attributes) {
                (*it_config_id_list).attributes[it.first] = it.second;
            }
        }
        it_asset_config_id_list ++;
    }

    // Get all secw document id for each configuration
    auto it_device_config_id_list = device_config_id_list.begin();
    while (it_device_config_id_list != device_config_id_list.end()) {
        std::set<std::string> document_type_list;
        tntdb::Statement st = conn.prepareCached(
            " SELECT BIN_TO_UUID(id_secw_document)"
            " FROM"
            "   t_bios_nut_configuration_secw_document"
            " WHERE id_nut_configuration = :id_nut_configuration"
        );
        tntdb::Result result = st.set("id_nut_configuration", (*it_device_config_id_list).id).select();
        for (auto &row: result) {
            std::string id_secw_document;
            row["BIN_TO_UUID(id_secw_document)"].get(id_secw_document);
            (*it_device_config_id_list).secwDocumentIdList.insert(id_secw_document);
        }
        it_device_config_id_list ++;
    }

    return device_config_id_list;
}

/**
 * @function get_candidate_config_list Get candidate configuration list of an asset
 * @param conn The connection to the database
 * @param asset_name The asset name to get configuration
 * @return The return configuration list of the asset
 */
DeviceConfigurationInfos get_candidate_config_list (tntdb::Connection& conn, const std::string& asset_name)
{
    const std::string request_where = " WHERE config.id_asset_element = :asset_id AND config.is_working = TRUE AND config.is_enabled = TRUE";
    return get_config_list(conn, request_where, asset_name);
}

/**
 * @function get_all_config_list Get all configuration list of an asset
 * @param conn The connection to the database
 * @param asset_name The asset name to get configuration
 * @return The return configuration list of the asset
 */
DeviceConfigurationInfos get_all_config_list (tntdb::Connection& conn, const std::string& asset_name)
{
// TBD: WORKAROUND SECURE DOCUMENT NOT INITIALISED - TO REMOVE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#if 1
    auto credentialsSNMPv1 = nutcommon::getCredentialsSNMPv1();
    auto credentialsSNMPv3 = nutcommon::getCredentialsSNMPv3();

    for (auto it = credentialsSNMPv1.begin(); it != credentialsSNMPv1.end(); it ++) {
        tntdb::Statement st = conn.prepareCached(
            " INSERT IGNORE INTO t_bios_secw_document"
            " (id_secw_document, id_secw_document_type)"
            " VALUES(UUID_TO_BIN(:id_secw_document), 'Snmpv1')"
        );
        st.set("id_secw_document", it->documentId).execute();
    }
    for (auto it = credentialsSNMPv3.begin(); it != credentialsSNMPv3.end(); it ++) {
        tntdb::Statement st = conn.prepareCached(
            " INSERT IGNORE INTO t_bios_secw_document"
            " (id_secw_document, id_secw_document_type)"
            " VALUES(UUID_TO_BIN(:id_secw_document), 'Snmpv1')"
        );
        st.set("id_secw_document", it->documentId).execute();
    }
#endif

    const std::string request_where = " WHERE config.id_asset_element =: asset_id";
    return get_config_list(conn, request_where, asset_name);
}

/**
 * @function is_config_working Get working value of a configuration
 * @param conn The connection to the database
 * @param config_id The configuration id
 * @return The return working value
 */
bool is_config_working (tntdb::Connection& conn, const size_t config_id)
{
    bool working_value;
    tntdb::Statement st = conn.prepareCached(
        " SELECT is_working"
        " FROM"
        "   t_bios_nut_configuration"
        " WHERE id_nut_configuration = :config_id"
    );

    tntdb::Row row = st.set("config_id", config_id).selectRow();
    row[0].get(working_value);
    return working_value;
}

/**
 * @function set_config_working Change working value of a configuration
 * @param conn The connection to the database
 * @param config_id The configuration id
 * @param working_value The new working value
 */
void set_config_working (tntdb::Connection& conn, const size_t config_id, const bool working_value)
{
    tntdb::Statement st = conn.prepareCached(
        " UPDATE"
        "   t_bios_nut_configuration"
        " SET"
        "   is_working = :working_value"
        " WHERE id_nut_configuration = :config_id"
    );

    st.set("config_id", config_id).set("working_value", working_value).execute();
}

/**
 * @function modify_config_priorities Change priorities of configuration list for an asset
 * @param conn The connection to the database
 * @param asset_name The asset name to change priorities of configuration list
 * @param configuration_id_list The list of configuration for priorities (first in the list is the highest priority)
 */
void modify_config_priorities (tntdb::Connection& conn, const std::string& asset_name, const std::vector<size_t>& configuration_id_list)
{
    const uint64_t asset_id = get_asset_id(conn, asset_name);

    tntdb::Statement st = conn.prepareCached(
        " SELECT id_nut_configuration, priority"
        " FROM"
        "   t_bios_nut_configuration"
        " WHERE id_asset_element = :asset_id"
    );

    tntdb::Result result = st.set("asset_id", asset_id).select();
    size_t config_id;
    int max_priority = -1;
    int priority = -1;
    std::vector<size_t> current_config_id_list;
    for (auto &row: result) {
        row["id_nut_configuration"].get(config_id);
        row["priority"].get(priority);
        if (priority > max_priority) max_priority = priority;
        current_config_id_list.push_back(config_id);
        // Test if config present in input configuration list
        auto it = std::find(configuration_id_list.begin(), configuration_id_list.end(), config_id);
        if (it == configuration_id_list.end()) {
            std::stringstream ss;
            ss << "Configuration id " << config_id << " not found in input configuration list for " << asset_name;
            throw std::runtime_error(ss.str());
        }
    }
    // Test if all input config present in database
    for (auto &configuration_id : configuration_id_list) {
        auto it = std::find(current_config_id_list.begin(), current_config_id_list.end(), configuration_id);
        if (it == current_config_id_list.end()) {
            std::stringstream ss;
            ss << "Configuration id " << configuration_id << " not found in database for " << asset_name;
            throw std::runtime_error(ss.str());
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
}

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
                      const nutcommon::DeviceConfiguration& key_value_asset_list)
{
    const uint64_t asset_id = get_asset_id(conn, asset_name);

    // Get max priority
    tntdb::Statement st = conn.prepareCached(
        " SELECT MAX(priority)"
        " FROM t_bios_nut_configuration"
        " WHERE id_asset_element = :asset_id"
    );
    tntdb::Row row = st.set("asset_id", asset_id).selectRow();
    int max_priority = -1;
    row[0].get(max_priority);

    // Insert new config
    st = conn.prepareCached(
        " INSERT INTO t_bios_nut_configuration"
        " (id_nut_configuration_type, id_asset_element, priority, is_enabled, is_working)"
        " VALUES"
        " (:config_type, :asset_id, :priority, :is_enabled, :is_working)"
    );
    st.set("config_type", config_type).
        set("asset_id", asset_id).
        set("priority", max_priority + 1).
        set("is_enabled", is_enabled).
        set("is_working", is_working).
        execute();
    long config_id = conn.lastInsertId();
    if (config_id < 0) {
        std::stringstream ss;
        ss << "No id when added new configuration for " << asset_name;
        throw std::runtime_error(ss.str());
    }

    // Insert document id list in configuration security wallet
    if (secw_document_id_list.size() > 0) {
        std::ostringstream s;
        s << " INSERT INTO t_bios_nut_configuration_secw_document"
          << " (id_nut_configuration, id_secw_document)"
          << " VALUES";
        int nb;
        for (nb = 0; nb < secw_document_id_list.size() - 1; nb ++) {
            s << " (:config_id, UUID_TO_BIN(:id_secw_document_" << nb << ")),";
        }
        s << " (:config_id, UUID_TO_BIN(:id_secw_document_" << nb << "))";
        st = conn.prepareCached(s.str());
        st = st.set("config_id", config_id);
        nb = 0;
        for (auto it = secw_document_id_list.begin(); it != secw_document_id_list.end(); ++it) {
            s.str("");
            s << "id_secw_document_" << nb ++;
            st = st.set(s.str(), *it);
        }
        st.execute();
    }

    // Insert the list of key values in the asset configuration attribute table
    if (key_value_asset_list.size() > 0) {
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
    }
    return config_id;
}

/**
 * @function remove_config Remove a configuration from database
 * @param conn The connection to the database
 * @param config_id The configuration id to remove
 */
void remove_config (tntdb::Connection& conn, const size_t config_id)
{
#if 0
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
#endif
    // Finally, remove configuration in t_bios_nut_configuration table
    tntdb::Statement st = conn.prepareCached(
        " DELETE"
        " FROM"
        "   t_bios_nut_configuration"
        " WHERE"
        "   id_nut_configuration = :config_id"
    );
    st.set("config_id", config_id).execute();
}

/**
 * @function get_all_configuration_types Get specific configuration information for each configuration type
 * @param conn The connection to the database
 * @return the specific configuration information for each configuration type
 */
DeviceConfigurationInfoDetails get_all_configuration_types (tntdb::Connection& conn)
{
    DeviceConfigurationInfoDetails config_info_list;

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
        default_values_list.emplace("driver", driver);
        default_values_list.emplace("port", port);

        // Get all document type according configuration type value
        std::set<std::string> document_type_list;
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
            document_type_list.insert(document_type);
        }
        DeviceConfigurationInfoDetail config_info;
        config_info.id = config_type;
        config_info.prettyName = config_name;
        config_info.defaultAttributes = default_values_list;
        config_info.secwDocumentTypes = document_type_list;
        config_info_list.emplace_back(config_info);
    }
    return config_info_list;
}

}

//  --------------------------------------------------------------------------
//  Self test of this class

// If your selftest reads SCMed fixture data, please keep it in
// src/selftest-ro; if your test creates file(void)system objects, please
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
    assert(system(command.c_str()) == 0);
    // Create working path for test
    buffer.str("");
    buffer << "mkdir " << run_working_path_test;
    command = buffer.str();
    assert(system(command.c_str()) == 0);
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
    file << "sleep 3\n";
    file << "mysql -u root -S " << run_working_path_test << "/mysqld.sock < /usr/share/bios/sql/mysql/initdb.sql\n";
    file << "for i in $(ls /usr/share/bios/sql/mysql/0*.sql | sort); do mysql -u root -S " << run_working_path_test << "/mysqld.sock < $i; done\n";
    file.close();

    // Change the right of the shell script for execution
    int ret = chmod(file_path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);

    // Execute the shell script
    assert(system(file_path.c_str()) == 0);

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
    file << "echo PID=$PID\n";
    file << "kill -9 $PID\n";
    file << "sleep 3\n";
    file << "rm -rf " << test_working_dir << "/db\n";
    file << "rm -rf " << run_working_path_test << "\n";
    file.close();

    // Change the right of the shell script for execution
    int ret = chmod(file_path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);

    // Execute the shell script
    assert(system(file_path.c_str()) == 0);

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

int test_get_priorities_base (tntdb::Connection& conn, int asset_id, std::vector<std::pair<size_t, size_t>>& configuration_id_list)
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
            size_t config_id;
            size_t priority;
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
    assert(getcwd(current_working_dir, FILENAME_MAX) != nullptr);
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
#if 0
            nutcommon::DeviceConfiguration device_config_list;
            std::cout << "\nTest get_candidate_config for " << t_asset_name[i] << ":" << std::endl;
            std::string device_config_id;
            int res = DBAssetsDiscovery::get_candidate_config(conn, t_asset_name[i], device_config_id, device_config_list);
            assert(res == 0);
            std::map<std::string, std::string> key_value_res = test_results[t_asset_name[i]].at(0);
            assert(key_value_res.size() == device_config_list.size());
            std::map<std::string, std::string>::iterator itr;
            for (itr = device_config_list.begin(); itr != device_config_list.end(); ++itr) {
                std::cout << "[" << itr->first << "] = " << itr->second << std::endl;
                assert(key_value_res.at(itr->first) == itr->second);
            }
#endif
        }

        // Test get_candidate_config_list function
        {
            DBAssetsDiscovery::DeviceConfigurationInfos device_config_id_list;
            std::cout << "\nTest get_candidate_configs for " << t_asset_name[i] << ":" << std::endl;
            device_config_id_list = DBAssetsDiscovery::get_candidate_config_list(conn, t_asset_name[i]);
            assert(test_results[t_asset_name[i]].size() == device_config_id_list.size());
            int nb_config = 0;
            auto it_config = device_config_id_list.begin();
            auto it_config_res = test_results[t_asset_name[i]].begin();
            while (it_config != device_config_id_list.end() && it_config_res != test_results[t_asset_name[i]].end()) {
                if (nb_config ++ != 0) std::cout << "<<<<<<<<<<<<" <<  std::endl;
                std::map<std::string, std::string> key_value_res = *it_config_res;
                assert(key_value_res.size() == (*it_config).attributes.size());
                auto it = (*it_config).attributes.begin();
                while (it != (*it_config).attributes.end()) {
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
            DBAssetsDiscovery::DeviceConfigurationInfos device_config_id_list;
            std::cout << "\nTest get_candidate_configs for " << t_asset_name[i] << ":" << std::endl;
            device_config_id_list = DBAssetsDiscovery::get_all_config_list(conn, t_asset_name[i]);
            std::cout << "size=" << device_config_id_list.size() << std::endl;
            assert(device_config_id_list.size() == 3);
        }
    }

    // Test get/set functions for configuration working value
    {
        bool value, initial_value;
        size_t config_id = 1;
        // Get initial value
        initial_value = DBAssetsDiscovery::is_config_working(conn, config_id);
        // Change current value
        DBAssetsDiscovery::set_config_working(conn, config_id, !initial_value);
        // Retry to change current value
        DBAssetsDiscovery::set_config_working(conn, config_id, !initial_value);
        // Get current value
        value = DBAssetsDiscovery::is_config_working(conn, config_id);
        assert(initial_value != value);
        // Restore initial value
        DBAssetsDiscovery::set_config_working(conn, config_id, initial_value);
        // Get current value
        value = DBAssetsDiscovery::is_config_working(conn, config_id);
        assert(initial_value == value);
    }

    // Test modify_config_priorities function
    {
        std::string asset_name = "ups-1";
        std::vector<std::pair<size_t, size_t>> config_priority_list;
        std::vector<size_t> init_config_id_list;
        std::vector<size_t> config_id_list;
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
        DBAssetsDiscovery::modify_config_priorities(conn, asset_name, config_id_list);
        // Read and check result
        config_priority_list.erase(config_priority_list.begin(), config_priority_list.end());
        assert(test_get_priorities_base(conn, asset_id, config_priority_list) == 0);
        int num_priority = 0;
        auto it_config_priority_list = config_priority_list.begin();
        auto it_config_id_list = config_id_list.begin();
        while (it_config_priority_list != config_priority_list.end() && it_config_id_list != config_id_list.end()) {
            assert((*it_config_priority_list).first == *it_config_id_list);
            assert(num_priority == it_config_priority_list->second);
            num_priority ++;
            it_config_priority_list ++;
            it_config_id_list ++;
        }
        // Restore previous priority order
        DBAssetsDiscovery::modify_config_priorities(conn, asset_name, init_config_id_list);
        // Read and check result
        config_priority_list.erase(config_priority_list.begin(), config_priority_list.end());
        assert(test_get_priorities_base(conn, asset_id, config_priority_list) == 0);
        num_priority = 0;
        it_config_priority_list = config_priority_list.begin();
        auto it_init_config_id_list = init_config_id_list.begin();
        while (it_config_priority_list != config_priority_list.end() && it_init_config_id_list != init_config_id_list.end()) {
            assert((*it_config_priority_list).first == *it_init_config_id_list);
            assert(num_priority == it_config_priority_list->second);
            num_priority ++;
            it_config_priority_list ++;
            it_init_config_id_list ++;
        }
    }

    // Test insert_config and remove_config function
    {
        std::map<std::string, std::string> key_value_asset_list = {{ "Key1", "Val1"}, { "Key2", "Val2"}, { "Key3", "Val3"}};
        std::set<secw::Id> secw_document_id_list = {{ "11111111-1111-1111-1111-000000000001" }};
        int config_type = 1;
        // Insert new config
        size_t config_id = DBAssetsDiscovery::insert_config(conn, "ups-1", config_type,
            true, true, secw_document_id_list, key_value_asset_list);
        assert(config_id > 0);
        // Remove inserted config
        DBAssetsDiscovery::remove_config(conn, config_id);
    }

    // Test get_all_configuration_types function
    {
        auto config_info_list = DBAssetsDiscovery::get_all_configuration_types(conn);
        auto it_config_info_list = config_info_list.begin();
        while (it_config_info_list != config_info_list.end() && it_config_info_list != config_info_list.end()) {
            std::cout << "--------------" << std::endl;
            std::cout << "type=" << it_config_info_list->id << std::endl;
            std::cout << "name=" << it_config_info_list->prettyName << std::endl;
            const auto& key_value_list = it_config_info_list->defaultAttributes;
            auto it_key_value_list = key_value_list.begin();
            while (it_key_value_list != key_value_list.end() && it_key_value_list != key_value_list.end()) {
                std::cout << "  " << it_key_value_list->first << "=" << it_key_value_list->second << std::endl;
                it_key_value_list ++;
            }
            const auto& document_type_list = it_config_info_list->secwDocumentTypes;
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