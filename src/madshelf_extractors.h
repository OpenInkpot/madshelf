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

#ifndef MADSHELF_EXTRACTORS_H
#define MADSHELF_EXTRACTORS_H

/*
 * libextractor-compatible extractors handling.
 */

#include <extractor.h>

typedef struct extractors_t
{
    void* handle;
    ExtractMethod method;
    struct extractors_t* next;
} extractors_t;

/*
 * Loads extractors from hardcoded directory (/usr/lib/madshelf/extractors) or
 * (if set) from ENV{EXTRACTORS_DIR}
 */
extractors_t* load_extractors();

void unload_extractors(extractors_t* extractors);

EXTRACTOR_KeywordList* extractor_get_keywords(extractors_t* extractors,
                                              const char* filename);


const char* extractor_get_last(const EXTRACTOR_KeywordType type,
                               const EXTRACTOR_KeywordList* keywords);

const char* extractor_get_first(const EXTRACTOR_KeywordType type,
                                const EXTRACTOR_KeywordList* keywords);

#endif
