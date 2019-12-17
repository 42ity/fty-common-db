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

#ifdef __cplusplus
namespace DBAssetsDiscovery {

/**
 * @function get_candidate_config Get first candidate configuration of an asset
 * @param asset_name The asset name to get configuration
 * @param config [out] The return configuration of the asset
 * @return {integer} 0 if no error else < 0
 */
int get_candidate_config (std::string asset_name, nutcommon::DeviceConfiguration& config);

/**
 * @function get_candidate_config Get all candidate configurations of an asset
 * @param asset_name The asset name to get configurations
 * @param configs [out] The return configurations of the asset
 * @return {integer} 0 if no error else < 0
 */
int get_candidate_configs (std::string asset_name, nutcommon::DeviceConfigurations& configs);


} // namespace

void fty_common_db_discovery_test (bool verbose);

#endif //namespace
#endif // FTY_COMMON_DB_DISCOVERY_H_INCLUDED
