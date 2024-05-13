#include "yexec.h"

/*
 * Execute a sub-program and wait for its termination.
 * See: https://wiki.sei.cmu.edu/confluence/pages/viewpage.action?pageId=87152177
 */
ystatus_t yexec(const char *command, yarray_t args, yarray_t env,
                ybin_t *out_memory, const char *out_file) {
	ystatus_t status = YENOERR;
	pid_t pid;
	int pipe_stdout[2], pipe_stderr[2];
	FILE *file = NULL;
	char **arg_list = NULL, **env_list = NULL;
	size_t i;

	if (!command)
		return (YENOEXEC);
	// open output file if needed
	if (out_file && !(file = fopen(out_file, "w"))) {
		return (YEIO);
	}
	// create pipes for communication
	if (pipe(pipe_stdout) == -1 || pipe(pipe_stderr) == -1) {
		//YLOG_ADD(YLOG_ERR, "Unable to create pipe.");
		return (YEPIPE);
	}
	// manage arguments
	size_t nbr_args = (args ? yarray_length(args) : 0) + 2;
	arg_list = malloc0(sizeof(char*) * nbr_args);
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
	// parent process: wait for child termination
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
		if (file)
			fclose(file);
	}
	int exec_status = 0;
	int res = waitpid(pid, &exec_status, 0);
	if (res == -1 || !WIFEXITED(exec_status) || WEXITSTATUS(exec_status) ||
	    WIFSIGNALED(exec_status)) {
		//YLOG_ADD(YLOG_WARN, "Unexpected end of process '%s' (%d).", command,
		//         WEXITSTATUS(exec_status));
		status = YENOEXEC;
	}
cleanup:
	free0(arg_list);
	free0(env_list);
	return (status);
}

