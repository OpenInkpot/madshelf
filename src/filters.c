#include <Ecore_File.h>
#include <Efreet_Mime.h>

#include "filters.h"
#include "handlers.h"
#include "utils.h"

bool is_visible(madshelf_filter_t filter, const char* filename)
{
    if(filter == MADSHELF_FILTER_NO)
        return true;

    if(!ecore_file_exists(filename)) return true;
    if(ecore_file_is_dir(filename)) return true;

    const char* mime_type = efreet_mime_type_get(filename);
    openers_t* openers = openers_get(mime_type);
    if(!openers)
        return false;

    if(filter == MADSHELF_FILTER_BOOKS)
        return openers->app_types & OPENERS_TYPE_BOOKS;
    if(filter == MADSHELF_FILTER_IMAGE)
        return openers->app_types & OPENERS_TYPE_IMAGE;
    if(filter == MADSHELF_FILTER_AUDIO)
        return openers->app_types & OPENERS_TYPE_AUDIO;

    die("is_visible: Unknown filter type: %d", filter);
}
