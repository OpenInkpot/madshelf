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

#ifndef DISKS_H
#define DISKS_H

#include <stdbool.h>
#include <Efreet.h>

typedef struct
{
    char* name;
    char* short_name;
    char* path;
    bool copy_target;
    bool is_removable;

    /* State */
    char* current_path;
} madshelf_disk_t;

typedef struct
{
    int n;
    madshelf_disk_t* disk;
} madshelf_disks_t;

madshelf_disks_t* fill_disks(Efreet_Ini* config);
madshelf_disks_t* fill_stub_disk();

madshelf_disk_t* find_disk(madshelf_disks_t* disks, const char* filename);

bool disk_mounted(madshelf_disk_t* disk);

void free_disks(madshelf_disks_t*);

#endif
