/*  =========================================================================
    fty_common_db_uptime - Uptime support function.

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
    fty_common_db_uptime - Uptime support function.
@discuss
@end
*/

#include <fty_common_db_uptime.h>

namespace DBUptime {
bool
get_dc_upses (const char *asset_name, zhash_t *hash)
{
    std::vector <std::string> list_ups{};
    std::function<void(const tntdb::Row&)> cb =     \
        [&list_ups](const tntdb::Row& row)
        {
            uint32_t type_id = 0;
            row["type_id"].get(type_id);

            uint32_t device_type_name = 0;
            row["subtype_id"].get(device_type_name);

            std::string device_name = "";
            row["name"].get(device_name);
            list_ups.push_back (device_name);

        };

    int64_t dc_id = DBAssets::name_to_asset_id (asset_name);
    if (dc_id < 0) {
        return false;
    }
    tntdb::Connection conn = tntdb::connectCached (DBConn::url);

    int rv = DBAssets::select_assets_by_container (conn,
                                         dc_id,
                                         {persist::asset_type::DEVICE},
                                         {persist::asset_subtype::UPS},
                                         "",
                                         "active",
                                         cb);

    if (rv != 0) {
        conn.close ();
        return false;
    }

    int i = 0;
    for (auto& ups : list_ups) {
        char key[14];
        sprintf (key,"ups%d", i);
        char *ups_name = strdup (ups.c_str ());
        zhash_insert (hash, key, (void*) ups_name);
        i++;

    }

    conn.close ();
    return hash;
}




}// namespace
