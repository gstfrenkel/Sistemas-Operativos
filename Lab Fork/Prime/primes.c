#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

const int ERROR = -1, EXITO = 0;
const int CANALES = 2;
const int FORK_HIJO = 0;
const int FIN_DE_ARCHIVO = 0;
const int PRIMER_NUMERO = 2;

void numeros_primos(int fds_original[CANALES]);

// Muestra por pantalla el primer número contenido en fds_original y se invoca
// recursivamente con aquellos números que no sean múltiplo del número impreso.
void
numeros_primos(int fds_original[CANALES])
{
	int primo = 0;

	close(fds_original[1]);

	if (read(fds_original[0], &primo, sizeof(primo)) == FIN_DE_ARCHIVO) {
		close(fds_original[0]);
		return;
	}

	printf("primo %i\n", primo);

	int fds[CANALES];
	int pipe_aux = pipe(fds);

	if (pipe_aux == ERROR) {
		perror("Error al abrir pipe.\n");
		return;
	}

	int fk = fork();

	if (fk != FORK_HIJO) {
		int recibido = 0;

		while (read(fds_original[0], &recibido, sizeof(recibido)) !=
		       FIN_DE_ARCHIVO) {
			if (recibido % primo != 0)
				write(fds[1], &recibido, sizeof(recibido));
		}

		close(fds_original[0]);
		close(fds[1]);
		close(fds[0]);

		wait(NULL);
	} else {
		close(fds_original[0]);

		numeros_primos(fds);
	}
}

int
main(int argc, char **argv)
{
	int ingresado = atoi(argv[1]);
	int fds[CANALES];
	int pipe_original = pipe(fds);

	if (pipe_original == ERROR) {
		perror("Error al abrir pipe.\n");
		return ERROR;
	}

	int fk = fork();

	if (fk != FORK_HIJO) {
		for (int i = PRIMER_NUMERO; i <= ingresado; i++)
			write(fds[1], &i, sizeof(i));

		close(fds[0]);
		close(fds[1]);

		wait(NULL);
	} else
		numeros_primos(fds);

	return EXITO;
}
