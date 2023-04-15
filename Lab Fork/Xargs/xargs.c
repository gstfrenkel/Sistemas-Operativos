#define _POSIX_C_SOURCE 200809L

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

const int ERROR = -1;
const int FORK_HIJO = 0;
const int FIN_DE_ARCHIVO = 0;
const size_t NARGS = 4;
const size_t MAX_CARACTERES = 256;
const char SALTO_DE_LINEA = '\n', FIN_DE_STRING = '\0';

void liberar_buffer(char **buffer);
void liberar_strings(char **oracion, size_t argumentos);
void liberar_oracion(char **oracion, size_t argumentos);
void formatear_strings(char **oracion, size_t argumentos);
size_t leer_lineas(char **oracion, size_t argumentos);

// Libera la referencia de buffer y la settea en NULL.
void
liberar_buffer(char **buffer)
{
	free(*buffer);
	(*buffer) = NULL;
}

// Libera los strings contenidos en oracion y los settea en NULL.
void
liberar_strings(char **oracion, size_t argumentos)
{
	for (size_t i = 0; i < argumentos; i++)
		liberar_buffer(&oracion[i + 1]);
}

// Se libera toda la memoria reservada para oracion, para lo cual oracion debe tener memoria reservada.
void
liberar_oracion(char **oracion, size_t argumentos)
{
	liberar_strings(oracion, argumentos);
	free(oracion);
}

// Lee argumentos pasados a través de stdin, y en función de lo ya leido en argv.
size_t
leer_lineas(char **oracion, size_t argumentos)
{
	size_t len = MAX_CARACTERES, i = 0;
	ssize_t bytes_leidos = 0;

	while (i < argumentos &&
	       (bytes_leidos = getline(&oracion[i + 1], &len, stdin)) != ERROR) {
		if (oracion[i + 1][bytes_leidos - 1] == SALTO_DE_LINEA)
			oracion[i + 1][bytes_leidos - 1] = FIN_DE_STRING;
		i++;
	}

	liberar_buffer(&oracion[i + 1]);
	return i;
}

int
main(int argc, char **argv)
{
	if (argc <= 1) {
		perror("Argumentos insuficientes.");
		return ERROR;
	}

	char **oracion = calloc((size_t) NARGS + 2, sizeof(char *));

	if (!oracion) {
		perror("Error al reservar memoria.");
		return ERROR;
	}

	oracion[0] = argv[1];

	while (leer_lineas(oracion, NARGS) != FIN_DE_ARCHIVO) {
		int fk = fork();

		if (fk != FORK_HIJO) {
			wait(NULL);

			liberar_strings(oracion,
			                NARGS);  // getline recomienda liberar los buffers utilizados para leer líneas.
		} else {
			execvp(oracion[0], oracion);

			perror("Exec falló.");
		}
	}

	liberar_oracion(oracion, NARGS);

	return 0;
}
