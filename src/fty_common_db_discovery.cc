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
#include <iomanip>
#include <map>

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
        log_error ("element %s not found", asset_name.c_str ());
        return -1;
    }
    catch (const std::exception &e)
    {
        log_error ("exception caught %s for element %s", e.what (), asset_name.c_str ());
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
        log_error ("element %s not found", asset_name.c_str());
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
            log_error("error during getting default configuration");
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
            log_error("error during getting asset configuration");
            return -4;
        }

        if (default_config_id != asset_config_id) {
            log_error("default config id different of asset config id");
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
        log_error("no connection to database");
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
        log_error ("element %s not found", asset_name.c_str());
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
            log_error("error during getting default configurations");
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
            log_error("error during getting asset configurations");
            return -4;
        }

        // TBD ???
        //if (default_config_id_list.size() != asset_config_id_list.size()) {
        //    log_error("error size of asset configurations is different of size of default configurations");
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
        log_error("no connection to database");
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
        log_error("no connection to database");
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
        log_error("no connection to database");
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
        log_error("no connection to database");
        return -1;
    }
    return set_config_working_ex(conn, config_id, working_value);
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

// FIXME: No necessary rights for this diectory
//std::string run_working_path_test("/var/run/fty_common_db_discovery");
std::string run_working_path_test("/home/admin/fty_common_db_discovery");

int start_database_test (std::string test_working_dir)
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

void stop_database_test (std::string test_working_dir)
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

int op_table_test (tntdb::Connection& conn, std::string request_table)
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

void del_data_tables_test (tntdb::Connection& conn)
{
    assert(op_table_test(conn, std::string("DELETE FROM t_bios_nut_configuration_default_attribute")) == 0);
    assert(op_table_test(conn, std::string("DELETE FROM t_bios_nut_configuration_attribute")) == 0);
    assert(op_table_test(conn, std::string("DELETE FROM t_bios_nut_configuration_type_secw_document_type_requirements")) == 0);
    assert(op_table_test(conn, std::string("DELETE FROM t_bios_nut_configuration_secw_document")) == 0);
    assert(op_table_test(conn, std::string("DELETE FROM t_bios_nut_configuration")) == 0);
    assert(op_table_test(conn, std::string("DELETE FROM t_bios_nut_configuration_type")) == 0);
    assert(op_table_test(conn, std::string("DELETE FROM t_bios_secw_document")) == 0);
    assert(op_table_test(conn, std::string("DELETE FROM t_bios_secw_document_type")) == 0);
    assert(op_table_test(conn, std::string("DELETE FROM t_bios_asset_element WHERE id_asset_element <> 1")) == 0);
}

void fty_common_db_discovery_test (bool verbose)
{
    printf (" * fty_common_db_discovery: ");

    // Get current directory
    char current_working_dir[FILENAME_MAX];
    getcwd(current_working_dir, FILENAME_MAX);
    // Set working test directory
    std::stringstream buffer;
    buffer << current_working_dir << "/" << SELFTEST_DIR_RW;
    std::string test_working_dir = buffer.str();

    // Create and start database for test
    start_database_test(test_working_dir);

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
    del_data_tables_test(conn);

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
    assert(op_table_test(conn, std::string(
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
    assert(op_table_test(conn, std::string(
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
    assert(op_table_test(conn, std::string(
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
    assert(op_table_test(conn, ss.str()) == 0);

    // Data for table t_bios_nut_configuration_secw_document
    assert(op_table_test(conn, std::string(
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
    assert(op_table_test(conn, std::string(
        " INSERT IGNORE INTO t_bios_nut_configuration_type_secw_document_type_requirements"
        " (id_nut_configuration_type, id_secw_document_type)"
        " VALUES"
        " (1, 'Snmpv1'),"
        " (2, 'Snmpv3'),"
        " (3, 'UserAndPassword'),"
        " (4, 'UserAndPassword'),"
        " (5, 'UserAndPassword'),"
        " (6, 'UserAndPassword')")) == 0
    );

    // Data for table t_bios_nut_configuration_attribute
    assert(op_table_test(conn, std::string(
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
    assert(op_table_test(conn, std::string(
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

    int asset_id = -1;
    int config_id = -1;
    int config_type = -1;

    int i = 0;
    /* for each asset */
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

    // Remove tables  data previously added
    del_data_tables_test(conn);

    // Stop and remove database
    stop_database_test(test_working_dir);

    printf ("\nEnd tests \n");
}


