#include <string.h>
#include <stdio.h>
#include <mpd/async.h>
#include "empd.h"

void
empd_enqueue_file(empd_connection_t* conn, const char *file)
{
    empd_file_queue_t* ptr = conn->queue;
    empd_file_queue_t* newitem = calloc(1, sizeof(empd_file_queue_t*));
    newitem->file = strdup(file);
    newitem->next = NULL;

    if(ptr)
    {
        while(ptr)
            ptr = ptr->next;
        ptr->next = newitem;
    }
    else
        conn->queue = newitem;
}



static void
empd_flush_queue(empd_connection_t* conn)
{
    char buf[1024];
    empd_file_queue_t* item = conn->queue;
    conn->queue = item->next;
    snprintf(buf, 1024, "file://%s", item->file);
    mpd_async_send_command(conn->async, "add", buf, NULL);
    free(item->file);
    free(item);
}

void
empd_enqueue_files(empd_connection_t* conn, Eina_List* list)
{
    char* item;
    EINA_LIST_FREE(list, item)
        empd_enqueue_file(conn, item);
    if(!conn->busy)
    {
        conn->busy = true;
        empd_flush_queue(conn);
    }
}

bool
empd_pending_events(empd_connection_t* conn)
{
    /* release lock, and re-run pending actions, we assube single-thread
    model and don't care about thread safety here */
    if(conn->queue)
    {
        empd_flush_queue(conn);
        return true;
    };
    return false;
}
