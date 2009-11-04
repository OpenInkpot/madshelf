#ifndef APP_DEFAULTS_H
#define APP_DEFAULTS_H

#include <Eina.h>

/* Result is not to be freed */
const char* appdef_get_preferred(const char* mime_type);

/* Only list is to be freed, not the strings */
Eina_List* appdef_get_list(const char* mime_type);

void appdef_set_default(const char* mime_type, const char* app);

void appdef_fini();

#endif
