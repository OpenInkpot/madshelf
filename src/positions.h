#ifndef POSITIONS_H
#define POSITIONS_H

struct positions_t;
typedef struct positions_t positions_t;

positions_t *
init_positions();

/*
 * Returns 0..100 for book in process of being read and -1 for book which is not
 * being read.
 */
int
get_position(positions_t *, const char *filename);

void
free_positions(positions_t *);

/*
 * Watch for changed positions
 */
typedef void (*positions_updated_cb)();

void *
positions_update_subscribe(positions_updated_cb cb, void *param);

void
positions_update_unsubscribe(void *);

#endif
