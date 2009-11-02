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

#ifndef FILE_CONTEXT_MENU_H
#define FILE_CONTEXT_MENU_H

#include <stdlib.h>
#include <Evas.h>

#include "madshelf.h"

typedef void (*draw_file_context_action_t)(const madshelf_state_t* state,
                                           Evas_Object* item,
                                           const char* filename,
                                           int item_num);

typedef void (*handle_file_context_action_t)(madshelf_state_t* state,
                                             const char* filename,
                                             int item_num,
                                             bool is_alt);

typedef void (*delete_file_context_action_t)(madshelf_state_t* state,
                                             const char* filename);

typedef void (*file_context_menu_closed_t)(madshelf_state_t* state,
                                           const char* filename,
                                           bool touched);

void open_file_context_menu(madshelf_state_t* state,
                            const char* title,
                            const char* filename,
                            int add_actions_num,
                            draw_file_context_action_t draw_action,
                            handle_file_context_action_t handle_action,
                            delete_file_context_action_t delete_action,
                            file_context_menu_closed_t closed);

void close_file_context_menu(Evas* canvas, bool touched);

#endif
