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
#ifndef DELETE_FILE_H
#define DELETE_FILE_H

#include "madshelf.h"

typedef void (*delete_file_handler_t)(void* param,
                                      const char* filename);

void delete_file_dialog(madshelf_state_t* state,
                        char* filename,
                        delete_file_handler_t handler,
                        void* param);

#endif
