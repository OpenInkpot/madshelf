#ifndef CURDIR_H
#define CURDIR_H

void curdir_init(const char* filename);
void curdir_set(const char* dir, const char* file);
char* curdir_get(const char* dir);
void curdir_fini();

#endif
