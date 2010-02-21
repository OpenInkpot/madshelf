#include "app_defaults.h"
#include <stdio.h>
#include <Efreet.h>

void pe(Eina_List* l)
{
    char* data;
    Eina_List* p;
    EINA_LIST_FOREACH(l, p, data)
        printf("%s\n", data);
    eina_list_free(l);
}

int main()
{
    printf("text/plain:\n");
    pe(appdef_get_list("text/plain"));
    printf("text/html:\n");
    pe(appdef_get_list("text/html"));

    printf("text/plain -> foo.desktop:\n");
    appdef_set_default("text/plain", "foo.desktop");
    pe(appdef_get_list("text/plain"));

    appdef_fini();
}
