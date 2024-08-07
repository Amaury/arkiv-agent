#include "yexec.h"

/** @const Default buffer size. */
#define READ_BUFFER_SIZE	4096

/* Execute a sub-program and wait for its termination. */
ystatus_t yexec(const char *command, yarray_t args, yarray_t env,
                ybin_t *out_memory, const char *out_file) {
	return yexec_stdin(command, args, env, NULL, NULL, NULL, out_memory, out_file);
}
/*
 * Execute a sub-program and wait for its termination.
 * See: https://wiki.sei.cmu.edu/confluence/pages/viewpage.action?pageId=87152177
 */
ystatus_t yexec_stdin(const char *command, yarray_t args, yarray_t env,
                      const char *stdin_str, ybin_t *stdin_bin, const char *stdin_file,
                      ybin_t *out_memory, const char *out_file) {
	ystatus_t status = YENOERR;
	pid_t pid;
	bool has_stdin = false;
	int pipe_stdin[2], pipe_stdout[2], pipe_stderr[2];
	FILE *file = NULL;
	char **arg_list = NULL, **env_list = NULL;
	size_t i;

	if (!command)
		return (YENOEXEC);
	// open output file if needed
	if (out_file && !(file = fopen(out_file, "w"))) {
		return (YEIO);
	}
	// check stdin parameters
	if (stdin_str && !stdin_str[0])
		stdin_str = NULL;
	if (stdin_str) {
		stdin_bin = NULL;
		stdin_file = NULL;
	}
	if (stdin_bin && ybin_empty(stdin_bin))
		stdin_bin = NULL;
	if (stdin_bin)
		stdin_file = NULL;
	if (stdin_file && !yfile_is_readable(stdin_file))
		stdin_file = NULL;
	if (stdin_str || stdin_bin || stdin_file)
		has_stdin = true;
	// create pipes for communication
	if (pipe(pipe_stdout) == -1 || pipe(pipe_stderr) == -1 ||
	    (has_stdin && pipe(pipe_stdin) == -1)) {
		//YLOG_ADD(YLOG_ERR, "Unable to create pipe.");
		return (YEPIPE);
	}
	// manage arguments
	size_t nbr_args = (args ? yarray_length(args) : 0) + 2;
	arg_list = malloc0(sizeof(char*) * nbr_args);
	if (!arg_list) {
		status = YENOMEM;
		goto cleanup;
	}
	arg_list[0] = (char*)command;
	i = 0;
	for (i = 0; args && i < yarray_length(args); ++i) {
		arg_list[i + 1] = args[i];
	}
	arg_list[i + 1] = NULL;
	// manage environment variables
	if (env) {
		size_t nbr_args = yarray_length(env) + 1;
		env_list = malloc0(sizeof(char*) * nbr_args);
		for (i = 0; i < yarray_length(env); ++i) {
			env_list[i] = env[i];
		}
		env_list[i] = NULL;
	}
	// create a sub-process
	if ((pid = fork()) < 0) {
		// fork error
		//YLOG_ADD(YLOG_ERR, "Unable to fork.");
		status = YENOEXEC;
		goto cleanup;
	} else if (!pid) {
		// child process: execute sub-program
		if (has_stdin) {
			while (dup2(pipe_stdin[0], STDIN_FILENO) == -1 && errno == EINTR)
				;
			close(pipe_stdin[0]);
			close(pipe_stdin[1]);
		}
		while (dup2(pipe_stdout[1], STDOUT_FILENO) == -1 && errno == EINTR)
			;
		while (dup2(pipe_stderr[1], STDERR_FILENO) == -1 && errno == EINTR)
			;
		close(pipe_stdout[0]);
		close(pipe_stdout[1]);
		close(pipe_stderr[0]);
		close(pipe_stderr[1]);
		close(0);
		execve(command, arg_list, env_list);
		exit(127);
	}
	/* parent process */
	// write data to sub-program's stdin
	if (has_stdin) {
		close(pipe_stdin[0]);
		if (stdin_str || stdin_bin) {
			size_t stdin_data_len = stdin_str ? strlen(stdin_str) : stdin_bin->bytesize;
			size_t written = 0;
			while (written < stdin_data_len) {
				const void *ptr = (stdin_str ? stdin_str : stdin_bin->data) + written;
				ssize_t result = write(pipe_stdin[1], ptr, stdin_data_len - written);
				if (result == -1) {
					if (errno == EINTR)
						continue;
					break;
				}
				written += result;
			}
			close(pipe_stdin[1]);
		} else {
			int fd = open(stdin_file, O_RDONLY);
			if (fd) {
				char buffer[READ_BUFFER_SIZE];
				ssize_t read_size;
				while ((read_size = read(fd, buffer, sizeof(buffer))) > 0) {
					size_t written = 0;
					while (written < sizeof(buffer)) {
						void *ptr = buffer + written;
						ssize_t result = write(pipe_stdin[1], ptr, sizeof(buffer) - written);
						if (result == -1) {
							if (errno == EINTR)
								continue;
							break;
						}
						written += result;
					}
				}
				close(pipe_stdin[1]);
				close(fd);
			}
		}
	}
	// get child output
	if (out_memory || out_file) {
		close(pipe_stdout[1]);
		//close(pipe_stderr[1]);
		char buffer[4096];
		for (;;) {
			ssize_t len = read(pipe_stdout[0], buffer, sizeof(buffer));
			if (len == -1 && errno == EINTR) {
				continue;
			} else if (len == -1) {
				status = YEPIPE;
				break;
			} else if (!len) {
				break;
			} else {
				if (out_memory)
					ybin_append(out_memory, buffer, len);
				if (out_file)
					fwrite(buffer, 1, len, file);
			}
		}
		close(pipe_stdout[0]);
		close(pipe_stderr[0]);
	}
	// wait for child termination
	int exec_status = 0;
	int res = waitpid(pid, &exec_status, 0);
	if (res == -1 || !WIFEXITED(exec_status) || WEXITSTATUS(exec_status) ||
	    WIFSIGNALED(exec_status)) {
		//YLOG_ADD(YLOG_WARN, "Unexpected end of process '%s' (%d).", command,
		//         WEXITSTATUS(exec_status));
		status = YEFAULT;
	}
cleanup:
	if (file)
		fclose(file);
	free0(arg_list);
	free0(env_list);
	return (status);
}

