#define _GNU_SOURCE
#define _POSIX_C_SOURCE >= 200809L

#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

const int ERROR = -1;
const int MAX_CARACTERES = 256;
const char *FLAG = "-i";
const char *DIR_ACTUAL = ".";
const char *DIR_ANTERIOR = "..";

char *hallar_string_a_buscar(int argc, char **argv, bool *hay_flag);
void buscar_patron(char *archivo,
                   char *path,
                   char *string_a_buscar,
                   bool case_sensitive);
void actualizar_path(char path[MAX_CARACTERES],
                     char *path_acumulado,
                     char *directorio);
void
buscar_archivos(int fds, char *path, char *string_a_buscar, bool case_sensitive);

// Devuelve el patrón sobre el que buscar los archivos, o error en caso de haber argumentos insuficientes.
char *
hallar_string_a_buscar(int argc, char **argv, bool *hay_flag)
{
	if (argc <= 1) {
		perror("Argumentos insuficientes.");
		return NULL;
	}

	if (strcmp(argv[1], FLAG) == 0)
		(*hay_flag) = true;

	if (argc == 2 && (*hay_flag)) {
		perror("Argumentos insuficientes.");
		return NULL;
	} else if (argc > 2)
		return argv[2];
	return argv[1];
}

// Muestra por pantalla la dirección del archivo si es que este contiene al string_a_buscar.
void
buscar_patron(char *archivo, char *path, char *string_a_buscar, bool case_sensitive)
{
	if (!case_sensitive) {
		if (strstr(archivo, string_a_buscar))
			printf("%s%s\n", path, archivo);
	} else {
		if (strcasestr(archivo, string_a_buscar))
			printf("%s%s\n", path, archivo);
	}
}

// Concatena las direcciones acumuladas hasta el momento con el nuevo directorio al que se ingresó.
void
actualizar_path(char path[MAX_CARACTERES], char *path_acumulado, char *directorio)
{
	strcpy(path, path_acumulado);
	strcat(path, directorio);
	strcat(path, "/");
}

// Recorre los archivos y directorios y muestra por pantalla los paths de aquellos que contengan el string_a_buscar.
void
buscar_archivos(int fds, char *path, char *string_a_buscar, bool case_sensitive)
{
	DIR *directorio = fdopendir(fds);
	struct dirent *entry;

	while ((entry = readdir(directorio))) {
		buscar_patron(entry->d_name, path, string_a_buscar, case_sensitive);

		if (entry->d_type == DT_DIR &&
		    strcmp(entry->d_name, DIR_ACTUAL) != 0 &&
		    strcmp(entry->d_name, DIR_ANTERIOR)) {
			int fds_directorio_actual =
			        openat(fds, entry->d_name, O_DIRECTORY);

			char path_actual[MAX_CARACTERES];
			actualizar_path(path_actual, path, entry->d_name);

			buscar_archivos(fds_directorio_actual,
			                path_actual,
			                string_a_buscar,
			                case_sensitive);
		}
	}
}

int
main(int argc, char **argv)
{
	bool hay_flag = false;
	char *string_a_buscar = hallar_string_a_buscar(argc, argv, &hay_flag);

	if (!string_a_buscar)
		return ERROR;

	DIR *base = opendir(DIR_ACTUAL);

	if (!base) {
		perror("Error al abrir directorio actual.");
		return ERROR;
	}

	int fds = dirfd(base);

	buscar_archivos(fds, "./", string_a_buscar, hay_flag);
	closedir(base);

	return 0;
}