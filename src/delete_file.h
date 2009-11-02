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
