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
#define RR 0x01
#define REJ 0x05

linkLayer ll;
struct termios oldtio, newtio;
int fd = 0;
int state = 0;
int nerros = 0;
int flag = 0;
int ntimeOuts;
int C = 0;
int R = 32;
int nI = 0;
int nREJ = 0;


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
			printf("FLAG->%d\n", input[0]);
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
}	
int llwrite (char *buf, int bufSize)
{
	int i = 0, k = 0, inputSize = bufSize + 5, res;
	int stuffedSize;
	char *input = malloc(sizeof(char) * (inputSize));
	char stuffed[MAX_PAYLOAD_SIZE * 2];
	char bcc2 = 0;
	char help = 0, rr_aux = 0, rej_aux = 0;

	input[0] = FLAG;
	input[1] = A_T;
	input[2] = C; // atualizar S se receçao da resposta foi bem sucedida
	input[3] = input[1] ^ input[2];

	rr_aux = RR ^ R;
	rej_aux = REJ ^ R; // nao tenho a certeza mas lets go with this

		C = C ^ 2;
		R = R ^ 32;

	// BCC2 creation
	// to be honest n sei de onde vem esta ideia do XOR, so we must ask teacher
		for (i = 0; i < bufSize; i++)
		{
			bcc2 ^= buf[i];
			input[i + HEADER_SIZE] = buf[i];
		}

		input[bufSize + HEADER_SIZE] = bcc2;

	// Byte Stuffing
	// n tenho de ir verificar bit a bit, porque a leitura é feita byte a byte
		stuffed[0] = FLAG;
		k = 0;
		for (i = 1; i < inputSize; i++)
		{
			if ((input[i] == 0x7e) || (input[i] == 0x7d))
			{
			stuffed[i + k] = 0x7d;
			k++;
			stuffed[i + k] = input[i] ^ 0x20;
			}
			else
			{
			stuffed[i + k] = input[i];
			}
		}

		stuffedSize = i + k;
		stuffed[stuffedSize] = FLAG;

		state = 0;
		k = 0;
		while (state < 2)
		{	

		switch (state)
		{

			case 0:

			if (k > ll.numTries)
			{
				printf("Nao recebeste dados nenhuns amigo, problems de envio\n");
				return -1;
				break;
			}

			if (k > 0)
			{
				printf("Retransmiting\n");
			}

			k++;
			sleep(0.00001);
			res = write(fd, stuffed, stuffedSize + 1);
			nI++;
			help = wait_for_answer();

			if (help == rr_aux)
			{
				state = 1;
				break;
			}
			if (help == rej_aux)
			{
				while (!read(fd, &help, 1)) // dar clear da last flag
				{
				}
				printf("Received REJ\n");
				state = 0;
				nREJ++;
				break;
			}

			state = 0;
			break;

		case 1:

			// add code to verify rejection or not
			while (!read(fd, &help, 1))
			{
			}

			if (help != FLAG)
			{
				printf("Nao recebemos last FLAG\n");
				nerros++;
				state = 0;
				break;
			}

			state++;
			return 1;

			break;
			}
		}	
}
	

int llread(char *packet)
{
	int packetSize = 0, res;
	char output[5];
	char bcc2 = 0;
	char aux;

	output[0] = FLAG;
	output[1] = A_T;
	output[4] = FLAG;

	state = 1;
	aux = wait_for_answer();

	if (aux == SET)
	{
		state = 1; // somente para ler a ultima flag
		llopen(ll);
		return 0;
	}

	if ((aux == 0) || (aux == 2))
	{
		nI++;
	}
	else
		return 0;

	R = (aux ^ 2) << 4;

	int j = 0;

	// como ja lemos anteriormente o HEADER, ja so temos DATA e BCC2 ate FLAG
	// FLAG also fica lida, need to tirar BCC2 de packet
	while (1)
	{
		sleep(0.00001);
		res = read(fd, &packet[j], 1);
		if (res == 0)
		{
			printf("ERRO READING DATA\n");
			break;
		}
		if (packet[j] == FLAG)
		{
			j++;
			break;
		}
		if (packet[j] == 0x7d)
		{
			sleep(0.00001);
			(void)read(fd, &packet[j], 1);
			packet[j] ^= 0x20;
		}
		bcc2 ^= packet[j];
		j++;
	}

	packetSize = j;
	// debugging code

	// flag += data_error(PROBABILITY);
	// printf("flag->%d\n", flag);
	// data_error adiciona erro ficticio ao cenas nos Dados para verifying
	if (bcc2 || flag == 1)
	{
		printf("Error in received (BCC2), sending REJ\n");
		nerros++;
		nREJ++;
		output[2] = REJ ^ R;
		output[3] = output[1] ^ output[2];
		res = write(fd, output, 5);
		return 0;
	}

	output[2] = RR ^ R;
	output[3] = output[1] ^ output[2];
	res = write(fd, output, 5);

	return packetSize - 2;
}

