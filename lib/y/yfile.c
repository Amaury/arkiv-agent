#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include "ystr.h"
#include "yfile.h"

/* Tell if a file exists. */
bool yfile_exists(const char *path) {
	struct stat st;

	if (!path || lstat(path, &st) || (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode)))
		return (false);
	return (true);
}
/* Tell if a file is a link. */
bool yfile_is_link(const char *path) {
	struct stat st;

	if (!path || lstat(path, &st) || !S_ISLNK(st.st_mode))
		return (false);
	return (true);
}
/* Tell if a directory exists. */
bool yfile_is_dir(const char *path) {
	struct stat st;

	if (!path || lstat(path, &st) || !S_ISDIR(st.st_mode))
		return (false);
	return (true);
}
/* Tell if a file is readable for the current user. */
bool yfile_is_readable(const char *path) {
	if (!path || access(path, R_OK))
		return (false);
	return (true);
}
/* Tell is a file is writable for the current user. */
bool yfile_is_writable(const char *path) {
	if (!path || access(path, W_OK))
		return (false);
	return (true);
}
/* Tell is a file is executable for the current user. */
bool yfile_is_executable(const char *path) {
	if (!path || access(path, X_OK))
		return (false);
	return (true);
}
/* Return the size of a readable file, in bytes. */
uint64_t yfile_get_size(const char *path) {
	struct stat st;

	if (!path || stat(path, &st))
		return (0);
	return ((uint64_t)st.st_size);
}
/* Create a path of directories. */
bool yfile_mkpath(const char *path, mode_t mode) {
	struct stat st;
	char *tmpPath;
	bool res = false;

	if (!path || !(tmpPath = strdup(path)))
		return (false);
	for (char *pt = tmpPath; *pt; ++pt) {
		if (*pt != '/' || pt == tmpPath)
			continue;
		*pt = '\0';
		if (!stat(tmpPath, &st)) {
			if (!S_ISDIR(st.st_mode))
				goto cleanup;
		} else if (mkdir(tmpPath, mode))
			goto cleanup;
		*pt = '/';
	}
	if ((!stat(tmpPath, &st) && S_ISDIR(st.st_mode)) || !mkdir(tmpPath, mode))
		res = true;
cleanup:
	free(tmpPath);
	return (res);
}
/* Create an empty file if it doesn't exist. */
bool yfile_touch(const char *path, mode_t file_mode, mode_t dir_mode) {
	char *tmpPath;
	bool res = false;
	char *pt;

	if (yfile_exists(path))
		return (true);
	if (!path || !(tmpPath = strdup(path)))
		return (false);
	for (pt = tmpPath + strlen(tmpPath) - 1; pt > path && *pt != '/'; --pt)
		;
	if (*pt == '/') {
		*pt = '\0';
		if (!yfile_mkpath(tmpPath, dir_mode))
			goto cleanup;
		*pt = '/';
	}
	if (open(path, O_WRONLY | O_CREAT, file_mode) == -1)
		goto cleanup;
	res = true;
cleanup:
	free(tmpPath);
	return (res);
}
/* Create a temporary file, randomly from a prefix path. */
char *yfile_tmp(const char *prefix) {
	ystr_t ys = NULL;
	int fd = -1;
	char *res = NULL;

	if (!prefix)
		return (NULL);
	ys = ys_printf(NULL, "%s-XXXXXX", prefix);
	if (!ys)
		return (NULL);
	fd = mkstemp(ys);
	if (fd == -1)
		goto err_cleanup;
	if (fchmod(fd, S_IRUSR | S_IWUSR))
		goto cleanup;
	res = ys_string(ys);
	if (!res)
		goto err_cleanup;
	goto cleanup;
err_cleanup:
	if (ys)
		unlink(ys);
cleanup:
	if (fd != -1)
		close(fd);
	ys_free(ys);
	return (res);
}
/* Read the full content of a file. */
ybin_t *yfile_get_contents(const char *path) {
	if (!yfile_is_readable(path))
		return (NULL);
	FILE *file = fopen(path, "r");
	if (!file)
		return (NULL);
	ybin_t *data = ybin_new();
	if (!data) {
		fclose(file);
		return (NULL);
	}
	char buffer[65535];
	size_t read_size;
	while ((read_size = fread(buffer, 1, 65535, file))) {
		if (ybin_append(data, buffer, read_size) != YENOERR) {
			ybin_delete(data);
			return (NULL);
		}
	}
	fclose(file);
	return (data);
}
/* Read the full textual content of a file. */
ystr_t yfile_get_string_contents(const char *path) {
	if (!yfile_is_readable(path))
		return (NULL);
	FILE *file;
	if (!(file = fopen(path, "r")))
		return (NULL);
	ystr_t ys = ys_new("");
	if (!ys) {
		fclose(file);
		return (NULL);
	}
	char buffer[65535];
	size_t read_size;
	while ((read_size = fread(buffer, 1, 65535, file))) {
		if (ys_append(&ys, buffer) != YENOERR) {
			ys_free(ys);
			return (NULL);
		}
	}
	fclose(file);
	return (ys);
}
/* Write some data in a file (mode 0600). */
bool yfile_put_contents(const char *path, ybin_t *data) {
	bool res = true;

	if (!path || !data || !data->data || !data->bytesize)
		return (false);
	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd == -1)
		return (false);
	ssize_t written = write(fd, data->data, data->bytesize);
	if (written == -1 || (size_t)written != data->bytesize)
		res = false;
	close(fd);
	return (res);
}
/* Write a string in a file (mode 600). */
bool yfile_put_string(const char *path, const char *str) {
	bool res = true;
	size_t len;

	if (!path)
		return (false);
	if (!str || !(len = strlen(str)))
		return (true);
	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd == -1)
		return (false);
	ssize_t written = write(fd, str, len);
	if (written == -1 || (size_t)written != len)
		res = false;
	close(fd);
	return (res);
}
/* Append some data at the end of a file. */
bool yfile_append_contents(const char *path, ybin_t *data) {
	bool res = true;

	if (!path || !data || !data->data || !data->bytesize)
		return (false);
	int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
	if (fd == -1)
		return (false);
	ssize_t written = write(fd, data->data, data->bytesize);
	if (written == -1 || (size_t)written != data->bytesize)
		res = false;
	close(fd);
	return (res);
}
/* Append a sting at the end of a file. */
bool yfile_append_string(const char *path, const char *str) {
	bool res = true;
	size_t len;

	if (!path)
		return (false);
	if (!str || !(len = strlen(str)))
		return (true);
	int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
	if (fd == -1)
		return (false);
	ssize_t written = write(fd, str, len);
	if (written == -1 || (size_t)written != len)
		res = false;
	close(fd);
	return (res);
}
/* Tell if a file contains a given file. */
bool yfile_contains(const char *path, const char *str) {
	ystr_t ys = yfile_get_string_contents(path);
	if (!ys)
		return (false);
	bool result = false;
	if (strstr(ys, str))
		result = true;
	ys_free(ys);
	return (result);
}

