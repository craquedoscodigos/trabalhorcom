#include "linklayer.h"

#define FLAG 0x7e
#define A_T 0x03
#define A_R 0x01
#define SET 0x03
#define DISC 0x0b
#define UA 0x07
#define MAX_BUFFER_SIZE 100
#define HEADER_SIZE 4
#define BBC1_T A_T ^ SET 
#define BCC1_R A_T ^ UA

linkLayer ll;
struct termios oldtio, newtio;
int fd = 0;
int state = 0;
int nerros = 0;
int flag = 0;
int ntimeOuts;

void timeout()
{
	printf("tempo excedido TIME OUT\n");
	state = 0;
	ntimeOuts++;
}

char state_machine()
{

	char input[HEADER_SIZE + 1], aux;
	int res = 0;

	(void)signal(SIGALRM, timeout);

	if (!state)
	{	

		state = 1;
		alarm(0);
		alarm(ll.timeOut);
	}

	while (state < 3)
	{
		switch (state)
		{
		case -1:
			return -2;

			
		case 0:
			return -1;

		case 1:
			while ((!read(fd, &input[0], 1)) && state)
			{
			}
			printf("FLAG->%d\n",input[0]);
			if (input[0] == FLAG)
			{
				state = 2;
				alarm(0);
			}
			else
			{
				printf("Primeira Flag mal recebida\n");
				nerros++;
				state = -1;
				alarm(0);
			}

			break;

		case 2:
			res = 0;
			while (!res && state)
			{
				sleep(0.00001);
				res = read(fd, &input[1], 3);
			}
			if ((input[3] != (input[1] ^ input[2])) || flag == 1)
			{
				printf("ERRO, BCC1 diferente\n");
				read(fd, &aux, 1); 
				nerros++;
				state = -1;
				break;
			}
			if (input[1] != A_T)
			{
				printf("ERRO, Address nao esta correto");
				state = -1;
				nerros++;
				break;
			}
			sleep(0.00001);
			state++;
			break;
		}
	}
	return input[2]; // enviamos o control para verificaçao
}
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

	char aux;

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
int k = 0, res = 0;
	char input[5];

	// activates timeOut interrupt

	switch (connectionParameters.role)
	{

	case TRANSMITTER:

		input[0] = FLAG;
		input[1] = A_T;
		input[2] = SET;
		input[3] = BBC1_T;
		input[4] = FLAG;

		printf("Transmissor enviou info\n");
		state = 0;
		while (state != 2)
		{

			switch (state)
			{

			// escrever informaçao e verificar
			case 0:
				if (k > connectionParameters.numTries)
				{
					printf("Nao recebeste dados, problemas de envio\n");
					return -1;
				}
				if (k > 0)
				{
					printf("Retransmissão\n");
				}

				k++;
				res = write(fd, input, 5);
				aux = state_machine();
				if (aux == UA)
				{
					state = 1;
					break;
				}

				state = 0;
				break;
			case 1:
				while (!read(fd, &aux, 1) && state)
				{
				}

				if (aux != FLAG)
				{
					printf("Nao recebemos last FLAG, trying again\n");
					nerros++;
					state = 0;
					break;
				}

				state++;
				printf("llopen Transmitter done successfully \n");
				return 1;

				break;
			}
		}

		return 1;

		// Ver return desta function quando ha erro, should I even count the tries???
		// HELPPPPPPPPP
	case RECEIVER:

		input[0] = FLAG;
		input[1] = A_T;
		input[2] = UA;
		input[3] = BCC1_R;
		input[4] = FLAG;

		printf("A começar RECEIVER\n");

		// control field definition
		while (state != 2)
		{
			// Receiver
			switch (state)
			{
			case 0:
				state = 1;
				aux = wait_for_answer();
				if (aux == SET)
				{
					state = 1;
					break;
				}

				state = 0;
				break;

			case 1:
				while (!read(fd, &aux, 1))
				{
				}

				if (aux != FLAG)
				{
					nerros++;
					printf("MISS last FLAG\n");
					state = 0;
					break;
				}

				write(fd, input, 5);////maximo de tamanho
				state++;
				printf("llopen rx done successfully\n");
				return 1;

				break;
			}
		}
		break;
	}
	

	return -1;
	close(fd);
}
	
