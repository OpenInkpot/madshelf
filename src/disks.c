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

#include "disks.h"

#include <stdbool.h>
#include <string.h>
#include <libintl.h>
#include <sys/stat.h>
#include <libgen.h>
#include <err.h>

#include "utils.h"

#define SECTION_PREFIX "X-Madshelf-Disk-"
#define SECTION_PREFIX_LEN (sizeof(SECTION_PREFIX)/sizeof(char) - 1)

static Eina_Bool
_count_cb(const Eina_Hash *hash, const void *key, void *data, void *fdata)
{
    int *count = (int *)fdata;
    if (!strncmp(SECTION_PREFIX, key, SECTION_PREFIX_LEN))
        (*count)++;
    return true;
}

static int
count_disks(Efreet_Ini *config)
{
    int count = 0;
    eina_hash_foreach(config->data, &_count_cb, &count);
    return count;
}

typedef struct
{
    Efreet_Ini *config;
    madshelf_disks_t *disks;
} _fill_param_t;

static void
cut_trailing_slash(char *s)
{
    char *c = strrchr(s, '/');
    while (c && c > s && *c == '/')
        *c-- = '\0';
}

static Eina_Bool
_fill_cb(const Eina_Hash *hash, const void *key, void *data, void *fdata)
{
    Efreet_Ini *config = ((_fill_param_t *)fdata)->config;
    madshelf_disks_t *disks = ((_fill_param_t *)fdata)->disks;
    madshelf_disk_t *disk = disks->disk + disks->n;

    if (!strncmp(SECTION_PREFIX, key, SECTION_PREFIX_LEN)) {
        efreet_ini_section_set(config, key);

        const char *sn = efreet_ini_localestring_get(config, "ShortName");
        if (!sn)
            disk->short_name = strdup((char *)key + SECTION_PREFIX_LEN);
        else
            disk->short_name = strdup(sn);

        const char *n = efreet_ini_localestring_get(config, "Name");
        if (!n)
            disk->name = strdup((char*)key + SECTION_PREFIX_LEN);
        else
            disk->name = strdup(n);

        const char *p = efreet_ini_localestring_get(config, "Path");
        if (!p)
            errx(1, "No path found for disk entry %s\n", (char*)key);

        disk->copy_target = efreet_ini_boolean_get(config, "Copy-Target");
        disk->is_removable = efreet_ini_boolean_get(config, "Removable");
        disk->path = strdup(p);

        cut_trailing_slash(disk->path);

        disks->n++;
    }

    return true;
}

madshelf_disks_t *
fill_disks(Efreet_Ini *config)
{
    madshelf_disks_t *disks = calloc(1, sizeof(madshelf_disks_t));
    if (!disks)
        return NULL;

    int n = count_disks(config);
    disks->n = 0;
    disks->disk = calloc(1, n * sizeof(madshelf_disk_t));
    if (!disks->disk) {
        free(disks);
        return NULL;
    }

    _fill_param_t param = { config, disks };

    eina_hash_foreach(config->data, &_fill_cb, &param);

    return disks;
}

madshelf_disks_t *
fill_stub_disk()
{
    madshelf_disks_t *disks = calloc(1, sizeof(madshelf_disks_t));
    if (!disks)
        return NULL;

    disks->n = 1;
    disks->disk = calloc(1, sizeof(madshelf_disk_t));
    disks->disk[0].name = strdup(gettext("Whole filesystem"));
    disks->disk[0].short_name = strdup("");
    disks->disk[0].path = strdup("/");

    return disks;
}

static bool is_path_prefix(const char *s, const char *t)
{
    if (s[0] == '/' && s[1] == '\0' && t[0] == '/')
        return true;

    while (*s)
        if (*s++ != *t++) return false;
    return *t == 0 || *t == '/';
}

bool
in_disk(madshelf_disk_t *disk, const char *filename)
{
    return is_path_prefix(disk->path, filename);
}

madshelf_disk_t *
find_disk(madshelf_disks_t *disks, const char *filename)
{
    int i;
    for (i = 0; i < disks->n; ++i)
        if (in_disk(disks->disk + i, filename))
            return disks->disk + i;
    return NULL;
}

static bool
is_mountpoint(const char *path)
{
    struct stat dir_s;
    struct stat par_s;

    if (0 != stat(path, &dir_s))
        return false;

    char *p = strdup(path);
    char *parent = dirname(p);
    if (stat(parent, &par_s) != 0) {
        free(p);
        return false;
    }
    free(p);

    return dir_s.st_dev != par_s.st_dev;
}

bool
disk_mounted(madshelf_disk_t *disk)
{
    if (!disk->is_removable)
        return 1;

    return is_mountpoint(disk->path);
}

void
free_disks(madshelf_disks_t *disks)
{
    int i;
    for (i = 0; i < disks->n; ++i) {
        free(disks->disk[i].path);
        free(disks->disk[i].short_name);
        free(disks->disk[i].name);
        free(disks->disk[i].current_path);
    }
    free(disks->disk);
    free(disks);
}
