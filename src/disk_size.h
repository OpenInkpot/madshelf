#ifndef DISK_SIZE_H
#define DISK_SIZE_H

/*
 * params:
 *  path - path to check for
 *  usage_output - format of used / total space
 *
 * author:
 *  Harris Bhatti
 */
void get_disk_usage(const char* path, char* usage_output);

#endif
