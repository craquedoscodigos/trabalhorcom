#include "linklayer.h"

linkLayer ll;
struct termios oldtio, newtio;
int fd = 0;



int llopen (linkLayer connectionParameters)
{
	
	ll.role = connectionParameters.role;
	ll.baudRate = connectionParameters.baudRate;
	ll.numTries = connectionParameters.numTries;
	ll.timeOut = connectionParameters.timeOut;
	sprintf(ll.serialPort, "%s", connectionParameters.serialPort);


	if (!fd)
	{
		fd = open (connectionParameters.serialPort,O_RDWR | O_NOCTTY );
		if (fd < 0)
		{
			perror(connectionParameters.serialPort);
			exit(-1);
		}
		if (tcgetattr(fd, &oldtio) == -1)
		{
			perror(" erro na função tcgetattr");
			exit(-1);
		}

	}
	while (1)
	{
		char dados_recebidos[100];
		char dados_trocados[100];

		// rx recebe os dados
		int bytes_lidos = read(fd, dados_recebidos, sizeof(dados_recebidos));

		if (bytes_lidos < 0)
		{
			printf("erro ao enviar os dados pela porta serial\n");
			exit (-1);

		}
		
		for (int i = 0; i < bytes_lidos; i++) {
            // inverte a ordem dos caracteres
            dados_trocados[i] = dados_recebidos[bytes_lidos - i - 1];
        }

		int bytes_enviados = write(fd, dados_trocados, bytes_lidos);
        if (bytes_enviados < 0) {
            printf("Erro ao enviar os dados pela porta serial.\n");
            break;
		}

	}
	close(fd);
	return 1;
}
