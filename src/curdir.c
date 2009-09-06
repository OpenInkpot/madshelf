/*
 * MadShelf - bookshelf application.
 *
 * Copyright © 2009 Mikhail Gusarov <dottedmag@dottedmag.net>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define _GNU_SOURCE

#include <stdbool.h>
#include <sqlite3.h>
#include <string.h>

#include "curdir.h"

/* Ugly */
static sqlite3* sqlite_db;
static sqlite3_stmt* stmt[2];

static bool _create_tables()
{
    const char* create_sql[] = {
        "PRAGMA SYNCHRONOUS = NORMAL",
        "DROP TABLE IF EXISTS meta",
        "DROP TABLE IF EXISTS selected_file",
        "CREATE TABLE meta (key TEXT PRIMARY KEY, value)",
        "CREATE TABLE selected_file (dir TEXT, file TEXT, UNIQUE (dir))",
        "INSERT INTO meta VALUES ('version', 0)",
    };

    int i;
    for(i = 0; i < sizeof(create_sql)/sizeof(create_sql[0]); ++i)
        if(SQLITE_OK != sqlite3_exec(sqlite_db, create_sql[i], NULL, NULL, NULL))
            return false;

    return true;
}

static int _update_schema()
{
    const char* query_version_sql = "SELECT value FROM meta WHERE key = 'version'";
    sqlite3_stmt* query_version_stmt;
    int ret = sqlite3_prepare_v2(sqlite_db, query_version_sql,
                                 strlen(query_version_sql), &query_version_stmt, NULL);

    if(ret != SQLITE_OK)
        return _create_tables();

    if(sqlite3_step(query_version_stmt) != SQLITE_ROW)
    {
        sqlite3_finalize(query_version_stmt);
        return _create_tables();
    }

    int version = sqlite3_column_int(query_version_stmt, 0);

    if(version != 0)
    {
        sqlite3_finalize(query_version_stmt);
        return false;
    }

    sqlite3_finalize(query_version_stmt);
    return true;
}

#define ST0 "INSERT OR REPLACE INTO selected_file VALUES (?1, ?2)"
#define ST1 "DELETE FROM selected_file WHERE dir = ?1"
#define ST2 "SELECT file FROM selected_file WHERE dir = ?1"

void curdir_init(const char* filename)
{
    sqlite3_open(filename, &sqlite_db);
    if(!_update_schema())
        return;

    sqlite3_prepare_v2(sqlite_db, ST0, strlen(ST0), &stmt[0], NULL);
    sqlite3_prepare_v2(sqlite_db, ST1, strlen(ST1), &stmt[1], NULL);
    sqlite3_prepare_v2(sqlite_db, ST2, strlen(ST2), &stmt[2], NULL);
}

void curdir_set(const char* dir, const char* file)
{
    if(file)
    {
        sqlite3_bind_text(stmt[0], 1, dir, strlen(dir), SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt[0], 2, file, strlen(file), SQLITE_TRANSIENT);
        sqlite3_step(stmt[0]);
        sqlite3_reset(stmt[0]);
    }
    else
    {
        sqlite3_bind_text(stmt[1], 1, dir, strlen(dir), SQLITE_TRANSIENT);
        sqlite3_step(stmt[1]);
        sqlite3_reset(stmt[1]);
    }
}

char* curdir_get(const char* dir)
{
    sqlite3_bind_text(stmt[2], 1, dir, strlen(dir), SQLITE_TRANSIENT);
    if(sqlite3_step(stmt[2]) != SQLITE_ROW)
    {
        sqlite3_reset(stmt[2]);
        return NULL;
    }
    char *res = strdup((char*)sqlite3_column_text(stmt[2], 0));
    sqlite3_reset(stmt[2]);
    return res;
}
