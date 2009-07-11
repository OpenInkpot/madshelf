/*
 * MadShelf - bookshelf application.
 *
 * Copyright (C) 2009 Mark Lajoie <quickhand@openinkpot.org>
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

#ifndef MADSHELF_H
#define MADSHELF_H

#include <stdbool.h>
#include <Ecore_Evas.h>
#include <Evas.h>

#include "disks.h"
#include "tags.h"

typedef enum
{
    MADSHELF_FILTER_NO,
    MADSHELF_FILTER_BOOKS,
    MADSHELF_FILTER_IMAGE,
    MADSHELF_FILTER_AUDIO,
} madshelf_filter_t;

typedef enum
{
    MADSHELF_SORT_NAME,
    MADSHELF_SORT_NAMEREV
} madshelf_sort_t;

/* Should match database.h::db_list_sort_t */
typedef enum
{
    MADSHELF_SORTEX_NAME,
    MADSHELF_SORTEX_NAMEREV,
    MADSHELF_SORTEX_DATE,
} madshelf_sortex_t;

typedef enum
{
    MADSHELF_LOC_OVERVIEW,
    MADSHELF_LOC_DIR,
    MADSHELF_LOC_FAVORITES_MENU,
    MADSHELF_LOC_FAVORITES,
    MADSHELF_LOC_RECENT,
} madshelf_loc_type_t;

typedef enum
{
    MADSHELF_FAVORITES_TYPE_ALL,
    MADSHELF_FAVORITES_TYPE_BOOKS,
    MADSHELF_FAVORITES_TYPE_IMAGE,
    MADSHELF_FAVORITES_TYPE_AUDIO,
} madshelf_favorites_type_t;

typedef struct madshelf_state_t madshelf_state_t;

typedef struct
{
    void (*free)(madshelf_state_t* state);
    void (*init_gui)(const madshelf_state_t* state);
    void (*update_gui)(const madshelf_state_t* state);
    bool (*key_down)(madshelf_state_t* state, Evas_Object* choicebox, Evas_Event_Key_Down* ev);
    void (*activate_item)(madshelf_state_t* state, Evas_Object* choicebox, int item_num, bool is_alt);
    void (*draw_item)(const madshelf_state_t* state, Evas_Object* item, int item_num);
    void (*fs_updated)(madshelf_state_t* state);
} madshelf_loc_t;

struct madshelf_state_t
{
    /* State */
    Ecore_Evas* win;
    Evas* canvas;
    madshelf_loc_t* loc;
    madshelf_filter_t filter;

    /* Preferences */
    madshelf_sort_t sort;
    bool show_hidden; /* FIXME: be more flexible */
    madshelf_sortex_t favorites_sort;
    madshelf_sortex_t recent_sort;

    /* Data */
    madshelf_disks_t* disks;
    tags_t* tags;
};

/* FIXME */

void go(madshelf_state_t* state, madshelf_loc_t* loc);

/* FIXME */
void item_clear(Evas_Object* item);

/* FIXME */
void set_favorites_sort(madshelf_state_t* state, madshelf_sortex_t sort);

/* FIXME */
void set_recent_sort(madshelf_state_t* state, madshelf_sortex_t sort);

/* FIXME */
void set_disk_current_path(madshelf_disk_t* disk, const char* path);

/* FIXME */
void set_show_hidden(madshelf_state_t* state, bool show_hidden);

/* FIXME */
void set_sort(madshelf_state_t* state, madshelf_sort_t sort);

typedef enum
{
    ICON_SORT_NONE = -1,
    ICON_SORT_NAME,
    ICON_SORT_NAMEREV,
    ICON_SORT_DATE,
} madshelf_icon_sort_t;

/* FIXME */
void set_sort_icon(const madshelf_state_t* state, madshelf_icon_sort_t icon);

#endif
