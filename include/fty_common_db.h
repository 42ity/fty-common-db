/*  =========================================================================
    fty-common-db - Provides common database tools for agents

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

typedef struct _fty_common_db_dbpath_t fty_common_db_dbpath_t;
#define FTY_COMMON_DB_DBPATH_T_DEFINED
typedef struct _fty_common_db_exception_t fty_common_db_exception_t;
#define FTY_COMMON_DB_EXCEPTION_T_DEFINED
typedef struct _fty_common_db_asset_t fty_common_db_asset_t;
#define FTY_COMMON_DB_ASSET_T_DEFINED
typedef struct _fty_common_db_asset_delete_t fty_common_db_asset_delete_t;
#define FTY_COMMON_DB_ASSET_DELETE_T_DEFINED
typedef struct _fty_common_db_asset_insert_t fty_common_db_asset_insert_t;
#define FTY_COMMON_DB_ASSET_INSERT_T_DEFINED
typedef struct _fty_common_db_asset_update_t fty_common_db_asset_update_t;
#define FTY_COMMON_DB_ASSET_UPDATE_T_DEFINED
typedef struct _fty_common_db_uptime_t fty_common_db_uptime_t;
#define FTY_COMMON_DB_UPTIME_T_DEFINED

#include "fty_common_db_asset.h"
#include "fty_common_db_asset_delete.h"
#include "fty_common_db_asset_insert.h"
#include "fty_common_db_asset_update.h"
#include "fty_common_db_dbpath.h"
#include "fty_common_db_defs.h"
#include "fty_common_db_exception.h"
#include "fty_common_db_uptime.h"
#include "fty_common_db_connection.h"
