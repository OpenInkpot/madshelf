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

#include <Efreet.h>

#include "run.h"
#include "handlers.h"
#include "fileinfo.h"

void run_default_handler(handlers_t* handlers, const char* filename)
{
     fileinfo_t* fileinfo = fileinfo_create(filename);
     Eina_List* handlers_list = handlers_get(handlers, fileinfo->mime_type);
     fileinfo_destroy(fileinfo);

     if(!handlers_list)
         return;

     Efreet_Desktop* handler = eina_list_data_get(handlers_list);

#ifdef OLD_ECORE
     Ecore_List* l = ecore_list_new();
     ecore_list_append(l, (void*)filename);
#else
     Eina_List* l = NULL;
     l = eina_list_append(l, filename);
#endif

     efreet_desktop_exec(handler, l, NULL);

#ifdef OLD_ECORE
     ecore_list_destroy(l);
#else
     eina_list_free(l);
#endif
}