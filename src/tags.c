/*
 * MadShelf - bookshelf application.
 *
 * Copyright (C) 2009 Mikhail Gusarov <dottedmag@dottedmag.net>
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

#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "tags.h"

typedef enum
{
    TAG_ADD,
    TAG_REMOVE,
    HAS_TAG,
    SELECT_TAG_BY_ORDER,
    SELECT_TAG_BY_NAME,
    SELECT_TAG_BY_NAMEREV,
    TAG_CLEAR,

    STMTS_T_SIZE,
} stmts_t;

static const char* stmts[] = {
    "INSERT OR REPLACE INTO tags VALUES (?1, ?2, ?3)",
    "DELETE FROM tags WHERE tag = ?1 AND filename = ?2",
    "SELECT COUNT(*) FROM tags WHERE tag = ?1 AND filename = ?2",
    "SELECT filename, _ROWID_ FROM tags WHERE tag = ?1 ORDER BY _ROWID_ DESC",
    "SELECT filename, _ROWID_ FROM tags WHERE tag = ?1 ORDER BY basename, filename",
    "SELECT filename, _ROWID_ FROM tags WHERE tag = ?1 ORDER BY basename DESC, filename DESC",
    "DELETE FROM tags",
};

struct tags_t
{
    sqlite3* sqlite_db;
    sqlite3_stmt* stmts[STMTS_T_SIZE];
};

typedef struct
{
    const char* sql;
    sqlite3_stmt** stmt;
} _stmt;

static int _create_tables(tags_t* db, char** errstr)
{
    const char* create_sql[] = {
        "DROP TABLE IF EXISTS meta",
        "DROP TABLE IF EXISTS tags",
        "CREATE TABLE meta (key TEXT PRIMARY KEY, value TEXT)",
        "CREATE TABLE tags (tag TEXT, filename TEXT, basename TEXT, UNIQUE (tag, filename))",
        "INSERT INTO meta VALUES ('version', 0)",
    };

    int i;
    for(i = 0; i < sizeof(create_sql)/sizeof(create_sql[0]); ++i)
        if(SQLITE_OK != sqlite3_exec(db->sqlite_db, create_sql[i], NULL, NULL, errstr))
            return false;

    return true;
}

static int _update_schema(tags_t* db, char** errstr)
{
    const char* query_version_sql = "SELECT value FROM meta WHERE key = 'version'";
    sqlite3_stmt* query_version_stmt;
    int ret = sqlite3_prepare_v2(db->sqlite_db, query_version_sql,
                                 strlen(query_version_sql), &query_version_stmt, NULL);

    if(ret != SQLITE_OK)
        return _create_tables(db, errstr);

    if(sqlite3_step(query_version_stmt) != SQLITE_ROW)
    {
        sqlite3_finalize(query_version_stmt);
        return _create_tables(db, errstr);
    }

    int version = sqlite3_column_int(query_version_stmt, 0);

    if(version != 0)
    {
        *errstr = strdup("Unknown (future?) DB schema version. Refusing to load.");
        sqlite3_finalize(query_version_stmt);
        return false;
    }

    sqlite3_finalize(query_version_stmt);
    return true;
}

tags_t* tags_init(const char* filename, char** errstr)
{
    tags_t* db = (tags_t*)calloc(1, sizeof(tags_t));
    if(!db)
    {
        if(errstr) *errstr = strdup(strerror(errno));
        tags_fini(db);
        return NULL;
    }

    int ret = sqlite3_open(filename, &db->sqlite_db);
    if(ret)
        goto err;

    if(!_update_schema(db, errstr))
    {
        tags_fini(db);
        return NULL;
    }

    int i;
    for(i = 0; i < STMTS_T_SIZE; ++i)
        if(SQLITE_OK != sqlite3_prepare_v2(db->sqlite_db, stmts[i],
                                           strlen(stmts[i]), &db->stmts[i], NULL))
            goto err;

    return db;

err:
    if(errstr) *errstr = strdup(sqlite3_errmsg(db->sqlite_db));
    tags_fini(db);
    return NULL;
}

void tags_fini(tags_t* db)
{
    if(db)
    {
        if(db->sqlite_db)
        {
            int i;
            for(i = 0; i < STMTS_T_SIZE && db->stmts[i]; ++i)
                sqlite3_finalize(db->stmts[i]);
            sqlite3_close(db->sqlite_db);
        }
        free(db);
    }
}

void tag_add(tags_t* db, const char* tag, const char* filename)
{
    sqlite3_stmt* s = db->stmts[TAG_ADD];
    sqlite3_bind_text(s, 1, tag, strlen(tag), SQLITE_TRANSIENT);
    sqlite3_bind_text(s, 2, filename, strlen(filename), SQLITE_TRANSIENT);
    char* b = basename(filename);
    sqlite3_bind_text(s, 3, b, strlen(b), SQLITE_TRANSIENT);
    sqlite3_step(s);
    sqlite3_reset(s);
}

void tag_remove(tags_t* db, const char* tag, const char* filename)
{
    sqlite3_stmt* s = db->stmts[TAG_REMOVE];
    sqlite3_bind_text(s, 1, tag, strlen(tag), SQLITE_TRANSIENT);
    sqlite3_bind_text(s, 2, filename, strlen(filename), SQLITE_TRANSIENT);
    sqlite3_step(s);
    sqlite3_reset(s);
}

bool has_tag(tags_t* db, const char* tag, const char* filename)
{
    sqlite3_stmt* s = db->stmts[HAS_TAG];

    sqlite3_bind_text(s, 1, tag, strlen(tag), SQLITE_TRANSIENT);
    sqlite3_bind_text(s, 2, filename, strlen(filename), SQLITE_TRANSIENT);
    sqlite3_step(s);

    int res = sqlite3_column_int(s, 0);
    sqlite3_reset(s);
    return res != 0;
}

void tag_list(tags_t* db, const char* tag, tags_sort_t sort, tags_list_t callback, void* param)
{
    int sort_stmt = sort == DB_SORT_ORDER
        ? SELECT_TAG_BY_ORDER
        : (sort == DB_SORT_NAME ? SELECT_TAG_BY_NAME
           : SELECT_TAG_BY_NAMEREV);

    sqlite3_stmt* s = db->stmts[sort_stmt];
    sqlite3_bind_text(s, 1, tag, strlen(tag), SQLITE_TRANSIENT);

    int res = sqlite3_step(s);
    while(res == SQLITE_ROW)
    {
        (*callback)((const char*)sqlite3_column_text(s, 0), sqlite3_column_int(s, 1), param);
        res = sqlite3_step(s);
    }
    sqlite3_reset(s);
}

void tag_clear(tags_t* db, const char* tag)
{
    sqlite3_stmt* s = db->stmts[TAG_CLEAR];

    sqlite3_bind_text(s, 1, tag, strlen(tag), SQLITE_TRANSIENT);
    sqlite3_step(s);
    sqlite3_reset(s);
}
