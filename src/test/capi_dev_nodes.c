/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/test/capi_dev_nodes.c $                                   */
/*                                                                        */
/* IBM Data Engine for NoSQL - Power Systems Edition User Library Project */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2014,2015                        */
/* [+] International Business Machines Corp.                              */
/*                                                                        */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */
#include <capi_dev_nodes.h>


#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

/*
 * Parse /proc/devices to find the major number of the character device
 * specified with name.
 *
 * Returns -1 on error or if name was not found.
 */
static int find_major_no(const char *name)
{
	const char *chardev_str = "Character devices:";
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	int major = -1;
	int in_char = 0;
	int tmp_major;
	char *tmp_name;

	fp = fopen("/proc/devices", "r");
	if (!fp) {
		perror("find_major_no: Unable to open /proc/devices");
		return -1;
	}
#if !defined(_AIX) && !defined(_MACOSX)
	while (getline(&line, &len, fp) != -1) {
		if (!strncmp(line, chardev_str, strlen(chardev_str)))
			in_char = 1;
		else if (in_char) {
			/* %ms requires glibc >= 2.7 */
			if (sscanf(line, "%i %ms", &tmp_major, &tmp_name) < 2) {
				in_char = 0;
				continue;
			}
			if (!strncmp(tmp_name, name, strlen(name))) {
				major = tmp_major;
				free(tmp_name);
				break;
			}
			free(tmp_name);
		}
	}
#endif /* !_AIX and !_MACOSX */
	if (major < 0)
		fprintf(stderr, "Unable to find %s in /proc/devices\n", name);

	free(line);
	fclose(fp);
	return major;
}

/*
 * If the device specified by path does not exist it will attempt to create it
 * by matching the major number of name from /proc/devices and the given minor
 * number.
 *
 * Note that if the device already exists this will NOT verify the major &
 * minor numbers.
 */
int create_dev(const char *path, const char *name, const int minor)
{
	struct stat sb;
	int result, major;

	result = stat(path, &sb);
	if (result < 0) {
		major = find_major_no(name);
		if (major < 0)
			return -1;
#if !defined(_AIX) && !defined(_MACOSX)
		if (mknod(path, 0600 | S_IFCHR, makedev(major, minor))) {
			perror("create_dev: Unable to create device");
			return -1;
		}
#else
		perror("create_dev: Unable to create device");
			return -1;
#endif
	}

	return 0;
}

/*
 * Open the device specified by path, creating it if it doesn't exist with
 * create_dev(name).
 */
int create_and_open_dev(const char *path, const char *name, const int minor)
{
	int fd;

	if (create_dev(path, name, minor))
		return -1;

	/* TODO: Enforce close on exec in driver: */
	fd = open(path, O_RDWR);
	if (fd < 0) {
		perror("create_and_open_dev: Unable to open device");
		return -1;
	}

	return fd;
}
