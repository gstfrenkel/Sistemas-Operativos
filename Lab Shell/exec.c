#include "exec.h"

const int OVERWRITE = 1, ERROR = -1, FK_HIJO = 0, CANALES = 2;
const int WRITE_FG = O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC;
const int READ_FG = O_RDONLY | O_CLOEXEC;

void run_exec_command(struct execcmd *cmd);
void redir(char file[FNAMESIZE], int flags, int fd_sobreescribir);
void run_redir_command(struct execcmd *cmd);
void run_right_cmd(struct cmd *right_cmd, int fds[CANALES], int *pipe_status);
void run_left_cmd(struct cmd *left_cmd, int fds[CANALES], int *pipe_status);
void run_pipe_command(struct pipecmd *cmd);

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	char key[MAXARGS];
	char value[MAXARGS];
	int idx = -1;

	for (int i = 0; i < eargc; i++) {
		idx = block_contains(eargv[i], '=');
		get_environ_key(eargv[i], key);
		get_environ_value(eargv[i], value, idx);
		setenv(key, value, OVERWRITE);
	}
}

// Settea las variables de entorno que acompañan al comando y ejecuta a este.
void
run_exec_command(struct execcmd *cmd)
{
	set_environ_vars(cmd->eargv, cmd->eargc);
	execvp(cmd->argv[0], cmd->argv);
	perror("Exec falló en exec command");
	_exit(ERR);
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	if (flags == WRITE_FG)
		return open(file, flags, S_IWUSR | S_IRUSR);
	return open(file, flags);
}

// Abre un file descriptor en función de file y reemplaza el fd_a_sobreescribir con el abierto.
void
redir(char file[FNAMESIZE], int flags, int fd_a_sobreescribir)
{
	if (strlen(file) <= 0)
		return;

	int fd = 0;

	if ((fd = open_redir_fd(file, flags)) == ERROR) {
		perror("Error al abrir file descriptor.");
		_exit(ERR);
	}

	close(fd_a_sobreescribir);

	if ((dup2(fd, fd_a_sobreescribir)) == ERROR) {
		perror("Error al sobreescribir file descriptor");
		close(fd);
		_exit(ERR);
	}
}

// Reemplaza uno de los primeros file descriptors del proceso con uno de los
// pasados en el comando, y ejecuta a este último.
void
run_redir_command(struct execcmd *cmd)
{
	redir(cmd->in_file, READ_FG, READ);
	redir(cmd->out_file, WRITE_FG, WRITE);

	if (strcmp(cmd->err_file, "&1") != READ)
		redir(cmd->err_file, WRITE_FG, ERR);
	else if (dup2(WRITE, ERR) == ERROR) {
		perror("Error al sobreescribir file descriptor");
		_exit(ERR);
	}

	cmd->type = EXEC;
	exec_cmd((struct cmd *) cmd);
}

// Lleva a cabo la ejecución del comando a la derecha del pipe (el que recibe la salida del comando a la izquierda).
void
run_right_cmd(struct cmd *right_cmd, int fds[CANALES], int *pipe_status)
{
	close(fds[WRITE]);

	if ((dup2(fds[READ], 0)) == ERROR) {
		close(fds[READ]);
		perror("Error al sobreescribir file descriptor");
		return;
	}

	close(fds[READ]);
	exec_cmd(right_cmd);
	perror("Exec falló en pipe derecho.");
	(*pipe_status) = 2;
	_exit(ERR);
}

// Lleva a cabo la ejecución del comando a la izquierda del pipe (el que envía la salida del comando a la derecha).
void
run_left_cmd(struct cmd *left_cmd, int fds[CANALES], int *pipe_status)
{
	close(fds[READ]);

	if ((dup2(fds[WRITE], 1)) == ERROR) {
		close(fds[WRITE]);
		perror("Error al sobreescribir file descriptor");
		return;
	}

	close(fds[WRITE]);
	exec_cmd(left_cmd);
	perror("Exec falló en pipe izquierdo.");
	(*pipe_status) = 2;
	_exit(ERR);
}

// Redirecciona la salida del comando izquierda y el comando derecho y lleva cabo las ejecuciones de estos.
void
run_pipe_command(struct pipecmd *cmd)
{
	int fk1, fk2;
	int fds[CANALES];
	int pipe_status = 0;

	if (pipe(fds) == ERROR) {
		perror("Error al abrir pipe");
		_exit(ERR);
	}

	if ((fk1 = fork()) == ERROR) {
		perror("Error al crear proceso hijo");
		close(fds[READ]);
		close(fds[WRITE]);
		_exit(ERR);
	}

	if (fk1 != FK_HIJO) {
		if ((fk2 = fork()) == ERROR) {
			perror("Error al crear proceso nieto");
			close(fds[WRITE]);
			close(fds[READ]);
			_exit(ERR);
		}

		if (fk2 != FK_HIJO) {
			close(fds[WRITE]);
			close(fds[READ]);
			waitpid(fk1, NULL, 0);
			waitpid(fk2, NULL, 0);
			free_command(parsed_pipe);
			exit(pipe_status);
		} else
			run_right_cmd(cmd->rightcmd, fds, &pipe_status);
	} else
		run_left_cmd(cmd->leftcmd, fds, &pipe_status);
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	switch (cmd->type) {
	case EXEC:
		// spawns a command
		run_exec_command((struct execcmd *) cmd);
		break;

	case BACK: {
		// runs a command in background
		exec_cmd(((struct backcmd *) cmd)->c);
		break;
	}

	case REDIR: {
		// changes the input/output/stderr flow
		//
		// To check if a redirection has to be performed
		// verify if file name's length (in the execcmd struct)
		// is greater than zero
		run_redir_command((struct execcmd *) cmd);
		break;
	}

	case PIPE: {
		// pipes two commands
		run_pipe_command((struct pipecmd *) cmd);
		break;
	}
	}
}
