/*
 * params:
 *  path - path to check for
 *  usage_output - format of used / total space
 *
 * author:
 *  Harris Bhatti
 */
void get_disk_usage(const char* path, char* usage_output)
{
	struct statvfs vfs_data;

	if (statvfs(path, &vfs_data) != 0)
	{
		strncpy(usage_output, "NA MB / NA MB", 19);
 	}
 	else
 	{
		// 9.54e-7 is precalculate 1/1024*1024 diving the
		// the number of bytes by this gives us megabytes.
 		// The factor is pre-calculate for some gain.
 		const double factor = vfs_data.f_bsize * 9.54e-7;
 		const double total = vfs_data.f_blocks * factor;
 		const double used = total - vfs_data.f_bfree * factor;

		sprintf(usage_output, "%gMB / %gMB", used, total);
	}
}
