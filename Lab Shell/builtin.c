#include "builtin.h"
#include "utils.h"

extern int status;

const char *CD_HOME = "cd";
const char *CD = "cd ";
static const int ERROR = -1;
const int MAX_PATH = 4096;  // Largo de path máximo y absoluto en Linux

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	return strcmp(cmd, "exit") == 0;
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	char *dir;

	if (strcmp(cmd, CD_HOME) == 0) {
		if (!(dir = getenv("HOME")))
			return 0;
	} else if (strncmp(cmd, CD, sizeof(char) * strlen(CD)) == 0) {
		dir = cmd + 3;
	} else
		return 0;

	if (chdir(dir) == ERROR) {
		perror("Error al actualizar directorio");
		status = 2;
		return 0;
	}

	char path[MAX_PATH];

	if (!getcwd(path, sizeof(char) * MAX_PATH)) {
		perror("Error al actualizar prompt");
		status = 2;
		return 1;
	}

	snprintf(prompt, PRMTLEN, "(%s)", path);
	status = 0;
	return 1;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (strcmp(cmd, "pwd") != 0)
		return 0;

	char path[MAX_PATH];

	if (!getcwd(path, sizeof(char) * MAX_PATH)) {
		perror("Error al actualizar prompt");
		status = 2;
		return 1;
	}

	printf_debug("%s\n", path);

	status = 0;
	return 1;
}
