/*
 *
 * Copyright (C) 2015 - 2020 Eaton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "fty_common_db_dbpath.h"
#include <fty_common_filesystem.h>
#include <fty_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <stdexcept>
#include <string.h>
#include <unistd.h> //getuid()

namespace DBConn {
static std::string s_get_dbpath_wo_trace()
{
    std::string s_url = std::string("mysql:db=box_utf8;user=") +
                        ((getenv("DB_USER") == nullptr) ? "root" : getenv("DB_USER")) +
                        ((getenv("DB_PASSWD") == nullptr) ? "" : std::string(";password=") + getenv("DB_PASSWD"));
    return s_url;
}

static std::string s_get_dbpath()
{
    std::string s_url = s_get_dbpath_wo_trace();
    log_debug("s_get_dbpath() : generated DB_URL=%s", s_url.c_str());
    return s_url;
}

std::string url = s_get_dbpath_wo_trace();

void dbpath()
{
    log_info("Updating db url with DB_USER=%s ..", (getenv("DB_USER") == nullptr) ? "root" : getenv("DB_USER"));
    url = s_get_dbpath();
}

// drop double quotes from a string
// needed for reading of db passwd file
// DB_USER="user" -> DB_USER=user
// modify buffer in place
static void s_dropdq(char* buffer)
{
    char* dq_ptr = nullptr;
    char* buf    = buffer;
    while ((dq_ptr = strchr(buf, '"')) != nullptr) {

        buf = dq_ptr;
        while (*dq_ptr) {
            *dq_ptr = *(dq_ptr + 1);
            dq_ptr++;
        }
    }
}

// read /etc/default/bios-db-rw and update url global variable
// @return true if OK, else false
bool dbreadcredentials()
{
    static char db_user[256];
    static char db_passwd[256];

    log_debug("dbreadcredentials : Reading %s ...", PASSWD_FILE);

    try {
        errno = 0;
        std::ifstream f{PASSWD_FILE};
        if (!f.is_open() || f.fail()) {
            throw std::runtime_error("Failed to open file");
        }

        // reset and setup db username/password

        memset(db_user, 0, sizeof(db_user));
        memset(db_passwd, 0, sizeof(db_passwd));

        f.getline(db_user, sizeof(db_user) - 1);
        f.getline(db_passwd, sizeof(db_passwd) - 1);
        if (!f.good() || f.fail()) {
            throw std::runtime_error("Failed to read file");
        }

        // remove double quotes
        s_dropdq(db_user);
        s_dropdq(db_passwd);
    }
    catch (const std::exception& e) {
        {
            uid_t uid = getuid();
            uid_t euid = geteuid();
            uid_t gid = getgid();
            uid_t egid = getegid();
            log_info("uid: %d (%d), gid: %d (%d)", uid, euid, gid, egid);
        }
        if (errno != 0) {
            log_error("errno: %d (%s)", errno, strerror(errno));
        }
        log_error("Exception: %s", e.what());
        return false;
    }

    log_debug("dbreadcredentials : setting envvars (%s)", db_user);
    putenv(db_user);
    putenv(db_passwd);

    // set url from env. vars
    dbpath();

    return true;
}

} // namespace DBConn
