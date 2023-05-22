#include "linklayer.h"

#define FLAG 0x7e
#define A_T 0x03
#define A_R 0x01
#define SET 0x03
#define DISC 0x0b
#define UA 0x07
#define MAX_BUFFER_SIZE 100

linkLayer ll;
struct termios oldtio, newtio;
int fd = 0;

void enviarTramaSET(int serialPort) {
    unsigned char tramaSET[5] = {FLAG, SET, SET, FLAG};
    write(fd, tramaSET, sizeof(tramaSET));
}

void enviarTramaUA(int serialPort) {
    unsigned char tramaUA[5] = {FLAG, UA, UA, FLAG};
    write(fd, tramaUA, sizeof(tramaUA));
}

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
	
	while (1) {
        unsigned char buffer[MAX_BUFFER_SIZE];
        int bytes_lidos = read(fd, buffer, sizeof(buffer));
        
        if (bytes_lidos < 0) {
            printf("Erro \n");
            break;
        }

        if (bytes_lidos > 0) {
            // Recebeu dados (RX)
            printf("Dados recebidos: ");
            for (int i = 0; i < bytes_lidos; i++) {
                printf("%02X ", buffer[i]);
            }
            printf("\n");
            
            // Verifica se é a trama SET
            if (buffer[0] == 0x7E && buffer[1] == 0x03 && buffer[2] == 0x03 && buffer[3] == 0x7E) {
                printf("Trama SET recebida!\n");
                enviarTramaUA(fd); 
            }

	}
	}
	close(fd);
	return 1;
}
