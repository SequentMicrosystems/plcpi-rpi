/*
 * plcpi.c:
 *	Command-line interface to the Raspberry
 *	Pi's plcpi card.
 *	Copyright (c) 2016-2024 Sequent Microsystem
 *	<http://www.sequentmicrosystem.com>
 ***********************************************************************
 *	Author: Alexandru Burcea
 ***********************************************************************
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "plcpi.h"
#include "comm.h"
#include "thread.h"
#include "cli.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

#define VERSION_BASE	(int)1
#define VERSION_MAJOR	(int)1
#define VERSION_MINOR	(int)0

#define UNUSED(X) (void)X      /* To avoid gcc/g++ warnings */

#define THREAD_SAFE
#define MOVE_PROFILE

u8 gHwVer = 0;

char *warranty =
	"	       Copyright (c) 2016-2024 Sequent Microsystems\n"
		"                                                             \n"
		"		This program is free software; you can redistribute it and/or modify\n"
		"		it under the terms of the GNU Leser General Public License as published\n"
		"		by the Free Software Foundation, either version 3 of the License, or\n"
		"		(at your option) any later version.\n"
		"                                    \n"
		"		This program is distributed in the hope that it will be useful,\n"
		"		but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		"		GNU Lesser General Public License for more details.\n"
		"			\n"
		"		You should have received a copy of the GNU Lesser General Public License\n"
		"		along with this program. If not, see <http://www.gnu.org/licenses/>.";

void usage(void)
{
	int i = 0;
	while (gCmdArray[i] != NULL)
	{
		if (gCmdArray[i]->name != NULL)
		{
			if (strlen(gCmdArray[i]->usage1) > 2)
			{
				printf("%s", gCmdArray[i]->usage1);
			}
			if (strlen(gCmdArray[i]->usage2) > 2)
			{
				printf("%s", gCmdArray[i]->usage2);
			}
		}
		i++;
	}
	printf("Where: <stack> = Board level id = 0..7\n");
	printf("Type plcpi -h <command> for more help\n");
}

int doBoardInit(int stack)
{
	int dev = 0;
	int add = 0;
	uint8_t buff[8];

	if ( (stack < 0) || (stack > 7))
	{
		printf("Invalid stack level [0..7]!");
		return ERROR;
	}
	add = stack + SLAVE_OWN_ADDRESS_BASE;
	dev = i2cSetup(add);
	if (dev == -1)
	{
		return ERROR;
	}
	if (ERROR == i2cMem8Read(dev, I2C_MEM_REVISION_HW_MAJOR_ADD, buff, 1))
	{
		printf("IO-PLUS id %d not detected\n", stack);
		return ERROR;
	}
	gHwVer = buff[0];
	return dev;
}

u8 getHwVer(void)
{
	return gHwVer;
}

int boardCheck(int stack)
{
	int dev = 0;
	int add = 0;
	uint8_t buff[8];

	if ( (stack < 0) || (stack > 7))
	{
		printf("Invalid stack level [0..7]!");
		return ERROR;
	}
	add = stack + SLAVE_OWN_ADDRESS_BASE;
	dev = i2cSetup(add);
	if (dev == -1)
	{
		return ERROR;
	}
	if (ERROR == i2cMem8Read(dev, I2C_MEM_REVISION_MAJOR_ADD, buff, 1))
	{

		return ERROR;
	}
	return OK;
}
int doHelp(int argc, char *argv[]);
const CliCmdType CMD_HELP = {"-h", 1, &doHelp,
	"\t-h		Display the list of command options or one command option details\n",
	"\tUsage:		plcpi -h    Display command options list\n",
	"\tUsage:		plcpi -h <param>   Display help for <param> command option\n",
	"\tExample:		plcpi -h rread    Display help for \"rread\" command option\n"};

int doHelp(int argc, char *argv[])
{
	int i = 0;
	if (argc == 3)
	{
		while (NULL != gCmdArray[i])
		{
			if (gCmdArray[i]->name != NULL)
			{
				if (strcasecmp(argv[2], gCmdArray[i]->name) == 0)
				{
					printf("%s%s%s%s", gCmdArray[i]->help, gCmdArray[i]->usage1,
						gCmdArray[i]->usage2, gCmdArray[i]->example);
					break;
				}
			}
			i++;
		}
		if (NULL == gCmdArray[i])
		{
			printf("Option \"%s\" not found\n", argv[2]);
			i = 0;
			while (NULL != gCmdArray[i])
			{
				if (gCmdArray[i]->name != NULL)
				{
					printf("%s", gCmdArray[i]->help);
					break;
				}
				i++;
			}
		}
	}
	else
	{
		i = 0;
		while (NULL != gCmdArray[i])
		{
			if (gCmdArray[i]->name != NULL)
			{
				printf("%s", gCmdArray[i]->help);
			}
			i++;
		}
	}
	return OK;
}

int doVersion(int argc, char *argv[]);
const CliCmdType CMD_VERSION = {"-v", 1, &doVersion,
	"\t-v		Display the plcpi command version number\n", "\tUsage:		plcpi -v\n",
	"", "\tExample:		plcpi -v  Display the version number\n"};

int doVersion(int argc, char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);
	printf("plcpi v%d.%d.%d Copyright (c) 2016 - 2023 Sequent Microsystems\n",
	VERSION_BASE, VERSION_MAJOR, VERSION_MINOR);
	printf("\nThis is free software with ABSOLUTELY NO WARRANTY.\n");
	printf("For details type: plcpi -warranty\n");
	return OK;
}

int doWarranty(int argc, char *argv[]);
const CliCmdType CMD_WAR = {"-warranty", 1, &doWarranty,
	"\t-warranty	Display the warranty\n", "\tUsage:		plcpi -warranty\n", "",
	"\tExample:		plcpi -warranty  Display the warranty text\n"};

int doWarranty(int argc UNU, char *argv[] UNU)
{
	printf("%s\n", warranty);
	return OK;
}

int doList(int argc, char *argv[]);
const CliCmdType CMD_LIST =
	{"-list", 1, &doList,
		"\t-list:		List all plcpi boards connected,return the # of boards and stack level for every board\n",
		"\tUsage:		plcpi -list\n", "", "\tExample:		plcpi -list display: 1,0 \n"};

int doList(int argc, char *argv[])
{
	int ids[8];
	int i;
	int cnt = 0;

	UNUSED(argc);
	UNUSED(argv);

	for (i = 0; i < 8; i++)
	{
		if (boardCheck(i) == OK)
		{
			ids[cnt] = i;
			cnt++;
		}
	}
	printf("%d board(s) detected\n", cnt);
	if (cnt > 0)
	{
		printf("Id:");
	}
	while (cnt > 0)
	{
		cnt--;
		printf(" %d", ids[cnt]);
	}
	printf("\n");
	return OK;
}

int doBoard(int argc, char *argv[]);
const CliCmdType CMD_BOARD = {"board", 2, &doBoard,
	"\tboard		Display the board status and firmware version number\n",
	"\tUsage:		plcpi <stack> board\n", "",
	"\tExample:		plcpi 0 board  Display vcc, temperature, firmware version \n"};

int doBoard(int argc, char *argv[])
{
	int dev = -1;
	u8 buff[4];
	int resp = 0;
	int temperature = 25;
	float voltage = 3.3;

	if (argc != 3)
	{
		printf("Invalid arguments number type \"plcpi -h\" for details\n");
		return (ARG_ERR);
	}
	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}
	resp = i2cMem8Read(dev, I2C_MEM_DIAG_TEMPERATURE_ADD, buff, 3);
	if (FAIL == resp)
	{
		printf("Fail to read board info!\n");
		return (FAIL);
	}
	temperature = buff[0];
	memcpy(&resp, &buff[1], 2);
	voltage = (float)resp / 1000; //read in milivolts

	resp = i2cMem8Read(dev, I2C_MEM_REVISION_HW_MAJOR_ADD, buff, 4);
	if (FAIL == resp)
	{
		printf("Fail to read board info!\n");
		return (FAIL);
	}
	printf(
		"Hardware %02d.%02d, Firmware %02d.%02d, CPU temperature %d C, voltage %0.2f V\n",
		(int)buff[0], (int)buff[1], (int)buff[2], (int)buff[3], temperature,
		voltage);
	return OK;
}
#ifdef HW_DEBUG
#define ERR_FIFO_MAX_SIZE 512
int doGetErrors(int argc, char *argv[]);
const CliCmdType CMD_ERR =
{
	"err",
	2,
	&doGetErrors,
	"\terr		Display the board logged errors \n",
	"\tUsage:		plcpi <stack> err\n",
	"",
	"\tExample:		plcpi 0 err  Display errors strings readed from the board \n"};

int doGetErrors(int argc, char *argv[])
{
	int dev = -1;
	u8 buff[ERR_FIFO_MAX_SIZE];
	int resp = 0;
	u16 size = 0;
	int retry = 0;

	if (argc != 3)
	{
		printf("Invalid arguments number type \"plcpi -h\" for details\n");
		return(FAIL);
	}
	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return(FAIL);
	}
	buff[0] = 1;
	resp = i2cMem8Write(dev, I2C_DBG_CMD, buff, 1);
	while ( (size == 0) && (retry < 10))
	{
		resp = i2cMem8Read(dev, I2C_DBG_FIFO_SIZE, buff, 2);
		if (FAIL != resp)
		{
			memcpy(&size, buff, 2);
		}
		retry++;
	}
	if (0 == size)
	{
		printf("Fail to read board error log, fifo empty!\n");
		return(FAIL);
	}
	if (size > ERR_FIFO_MAX_SIZE)
	{
		size = ERR_FIFO_MAX_SIZE;
	}
	resp = i2cMem8Read(dev, I2C_DBG_FIFO_ADD, buff, size);
	if (FAIL == resp)
	{
		printf("Fail to read board error log, fifo read %d bytes error!\n",
		(int)size);
		return(FAIL);
	}
	buff[size - 1] = 0;

	printf("%s\n", (char*)buff);
	for (retry = 0; retry < size; retry++)
	{
		printf("%02x ", buff[retry]);
		if (0 == ((retry + 1) % 16))
		{
			printf("\n");
		}
	}
	printf("\n");
}
#endif

int relayChSet(int dev, u8 channel, OutStateEnumType state)
{
	int resp = 0;
	u8 buff[2];

	if ( (channel < CHANNEL_NR_MIN) || (channel > RELAY_CH_NR_MAX))
	{
		printf("Invalid relay nr!\n");
		return ERROR;
	}
	if (FAIL == i2cMem8Read(dev, I2C_MEM_RELAY_VAL_ADD, buff, 1))
	{
		return FAIL;
	}

	switch (state)
	{
	case OFF:
		buff[0] &= ~ (1 << (channel - 1));
		resp = i2cMem8Write(dev, I2C_MEM_RELAY_VAL_ADD, buff, 1);
		break;
	case ON:
		buff[0] |= 1 << (channel - 1);
		resp = i2cMem8Write(dev, I2C_MEM_RELAY_VAL_ADD, buff, 1);
		break;
	default:
		printf("Invalid relay state!\n");
		return ERROR;
		break;
	}
	return resp;
}

int relayChGet(int dev, u8 channel, OutStateEnumType *state)
{
	u8 buff[2];

	if (NULL == state)
	{
		return ERROR;
	}

	if ( (channel < CHANNEL_NR_MIN) || (channel > RELAY_CH_NR_MAX))
	{
		printf("Invalid relay nr!\n");
		return ERROR;
	}

//	if (FAIL == i2cMem8Read(dev, I2C_MEM_RELAY_VAL_ADD, buff, 1))
//	{
//		return ERROR;
//	}
	if (OK != i2cReadByteAS(dev, I2C_MEM_RELAY_VAL_ADD, buff))
	{
		return ERROR;
	}
	if (buff[0] & (1 << (channel - 1)))
	{
		*state = ON;
	}
	else
	{
		*state = OFF;
	}
	return OK;
}

int relaySet(int dev, int val)
{
	u8 buff[2];

	buff[0] = 0xff & val;

	return i2cMem8Write(dev, I2C_MEM_RELAY_VAL_ADD, buff, 1);
}

int relayGet(int dev, int *val)
{
	u8 buff[2];

	if (NULL == val)
	{
		return ERROR;
	}
	if (OK != i2cReadByteAS(dev, I2C_MEM_RELAY_VAL_ADD, buff))
	{
		return ERROR;
	}
	*val = buff[0];
	return OK;
}

int doRelayWrite(int argc, char *argv[]);
const CliCmdType CMD_RELAY_WRITE = {"relwr", 2, &doRelayWrite,
	"\trelwr:		Set relays On/Off\n",
	"\tUsage:		plcpi <stack> relwr <channel> <on/off>\n",
	"\tUsage:		plcpi <stack> relwr <value>\n",
	"\tExample:		plcpi 0 relwr 2 1; Set Relay #2 on Board #0 On\n"};

int doRelayWrite(int argc, char *argv[])
{
	int pin = 0;
	OutStateEnumType state = STATE_COUNT;
	int val = 0;
	int dev = 0;
	OutStateEnumType stateR = STATE_COUNT;
	int valR = 0;
	int retry = 0;

	if ( (argc != 5) && (argc != 4))
	{
		printf("%s", CMD_RELAY_WRITE.usage1);
		printf("%s", CMD_RELAY_WRITE.usage2);
		return (FAIL);
	}

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}
	if (argc == 5)
	{
		pin = atoi(argv[3]);
		if ( (pin < CHANNEL_NR_MIN) || (pin > RELAY_CH_NR_MAX))
		{
			printf("Relay number value out of range\n");
			return (FAIL);
		}

		/**/if ( (strcasecmp(argv[4], "up") == 0)
			|| (strcasecmp(argv[4], "on") == 0))
			state = ON;
		else if ( (strcasecmp(argv[4], "down") == 0)
			|| (strcasecmp(argv[4], "off") == 0))
			state = OFF;
		else
		{
			if ( (atoi(argv[4]) >= STATE_COUNT) || (atoi(argv[4]) < 0))
			{
				printf("Invalid relay state!\n");
				return (FAIL);
			}
			state = (OutStateEnumType)atoi(argv[4]);
		}

		retry = RETRY_TIMES;

		while ( (retry > 0) && (stateR != state))
		{
			if (OK != relayChSet(dev, pin, state))
			{
				printf("Fail to write relay\n");
				return (FAIL);
			}
			if (OK != relayChGet(dev, pin, &stateR))
			{
				printf("Fail to read relay\n");
				return (FAIL);
			}
			retry--;
		}
#ifdef DEBUG_I
		if(retry < RETRY_TIMES)
		{
			printf("retry %d times\n", 3-retry);
		}
#endif
		if (retry == 0)
		{
			printf("Fail to write relay\n");
			return (FAIL);
		}
	}
	else
	{
		val = atoi(argv[3]);
		if (val < 0 || val > 255)
		{
			printf("Invalid relay value\n");
			return (FAIL);
		}

		retry = RETRY_TIMES;
		valR = -1;
		while ( (retry > 0) && (valR != val))
		{

			if (OK != relaySet(dev, val))
			{
				printf("Fail to write relay!\n");
				return (FAIL);
			}
			if (OK != relayGet(dev, &valR))
			{
				printf("Fail to read relay!\n");
				return (FAIL);
			}
		}
		if (retry == 0)
		{
			printf("Fail to write relay!\n");
			return (FAIL);
		}
	}
	return OK;
}

int doRelayRead(int argc, char *argv[]);
const CliCmdType CMD_RELAY_READ = {"relrd", 2, &doRelayRead,
	"\trelrd:		Read relays status\n",
	"\tUsage:		plcpi <stack> relrd <channel>\n",
	"\tUsage:		plcpi <stack> relrd\n",
	"\tExample:		plcpi 0 relrd 2; Read Status of Relay #2 on Board #0\n"};

int doRelayRead(int argc, char *argv[])
{
	int pin = 0;
	int val = 0;
	int dev = 0;
	OutStateEnumType state = STATE_COUNT;

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}

	if (argc == 4)
	{
		pin = atoi(argv[3]);
		if ( (pin < CHANNEL_NR_MIN) || (pin > RELAY_CH_NR_MAX))
		{
			printf("Relay number value out of range!\n");
			return (FAIL);
		}

		if (OK != relayChGet(dev, pin, &state))
		{
			printf("Fail to read!\n");
			return (FAIL);
		}
		if (state != 0)
		{
			printf("1\n");
		}
		else
		{
			printf("0\n");
		}
	}
	else if (argc == 3)
	{
		if (OK != relayGet(dev, &val))
		{
			printf("Fail to read!\n");
			return (FAIL);
		}
		printf("%d\n", val);
	}
	else
	{
		printf("%s", CMD_RELAY_READ.usage1);
		printf("%s", CMD_RELAY_READ.usage2);
		return (FAIL);
	}
	return OK;
}

int doRelayTest(int argc, char *argv[]);
const CliCmdType CMD_TEST = {"reltest", 2, &doRelayTest,
	"\treltest:	Turn ON and OFF the relays until press a key\n",
	"\tUsage:		plcpi <stack> reltest\n", "", "\tExample:		plcpi 0 reltest\n"};

int doRelayTest(int argc, char *argv[])
{
	int dev = 0;
	int i = 0;
	int retry = 0;
	int relVal;
	int valR;
	int relayResult = 0;
	FILE *file = NULL;
	const u8 relayOrder[8] = {1, 2, 3, 4, 5, 6, 7, 8};

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}
	if (argc == 4)
	{
		file = fopen(argv[3], "w");
		if (!file)
		{
			printf("Fail to open result file\n");
			//return -1;
		}
	}
//relay test****************************
	if (strcasecmp(argv[2], "reltest") == 0)
	{
		relVal = 0;
		printf(
			"Are all relays and LEDs turning on and off in sequence?\nPress y for Yes or any key for No....");
		startThread();
		while (relayResult == 0)
		{
			for (i = 0; i < 8; i++)
			{
				relayResult = checkThreadResult();
				if (relayResult != 0)
				{
					break;
				}
				valR = 0;
				relVal = (u8)1 << (relayOrder[i] - 1);

				retry = RETRY_TIMES;
				while ( (retry > 0) && ( (valR & relVal) == 0))
				{
					if (OK != relayChSet(dev, relayOrder[i], ON))
					{
						retry = 0;
						break;
					}

					if (OK != relayGet(dev, &valR))
					{
						retry = 0;
					}
				}
				if (retry == 0)
				{
					printf("Fail to write relay\n");
					if (file)
						fclose(file);
					return (FAIL);
				}
				busyWait(150);
			}

			for (i = 0; i < 8; i++)
			{
				relayResult = checkThreadResult();
				if (relayResult != 0)
				{
					break;
				}
				valR = 0xff;
				relVal = (u8)1 << (relayOrder[i] - 1);
				retry = RETRY_TIMES;
				while ( (retry > 0) && ( (valR & relVal) != 0))
				{
					if (OK != relayChSet(dev, relayOrder[i], OFF))
					{
						retry = 0;
					}
					if (OK != relayGet(dev, &valR))
					{
						retry = 0;
					}
				}
				if (retry == 0)
				{
					printf("Fail to write relay!\n");
					if (file)
						fclose(file);
					return (FAIL);
				}
				busyWait(150);
			}
		}
	}
	else
	{
		usage();
		return (FAIL);
	}
	if (relayResult == YES)
	{
		if (file)
		{
			fprintf(file, "Relay Test ............................ PASS\n");
		}
		else
		{
			printf("Relay Test ............................ PASS\n");
		}
	}
	else
	{
		if (file)
		{
			fprintf(file, "Relay Test ............................ FAIL!\n");
		}
		else
		{
			printf("Relay Test ............................ FAIL!\n");
		}
	}
	if (file)
	{
		fclose(file);
	}
	relaySet(dev, 0);
	return OK;
}

const CliCmdType CMD_GPIO_ENC_CNT_READ = {"cntencrd", 2, &doGpioEncoderCntRead,
	"\tcntencrd:	Read PLC Pi08 encoder count \n",
	"\tUsage:		plcpi <stack> cntencrd \n", "",
	"\tExample:		plcpi 0 cntencrd ; Read couter of the PLC Pi08 encoder \n"};

const CliCmdType CMD_GPIO_ENC_CNT_RESET = {"cntencrst", 2,
	&doGpioEncoderCntReset, "\tcntencrst:	Reset PLC Pi08 encoder count \n",
	"\tUsage:		plcpi <stack> cntencrst \n", "",
	"\tExample:		plcpi 0 cntencrst 2; Reset contor of the PLC Pi08 encoder\n"};

const CliCmdType CMD_OPTO_OD_CMD_SET =
	{"incmd", 2, &doInCmdSet,
		"\tincmd:	Set PLC Pi08 command for input channel \n",
		"\tUsage:		plcpi <stack> incmd <inCh> <outCh> <cnt>\n", "",
		"\tExample:		plcpi 0 incmd 2 1 1000; PLC Pi08 od channel 1 will start 1000 pulses on rising edge of the input channel 2\n"};

const CliCmdType CMD_OPTO_READ =
	{"optrd", 2, &doOptoRead, "\toptrd:		Read optocoupled inputs status\n",
		"\tUsage:		plcpi <stack> optrd <channel>\n",
		"\tUsage:		plcpi <stack> optrd\n",
		"\tExample:		plcpi 0 optrd 2; Read Status of Optocoupled input ch #2 on Board #0\n"};

const CliCmdType CMD_OPTO_EDGE_WRITE =
	{"optedgewr", 2, &doOptoEdgeWrite,
		"\toptedgewr:	Set optocoupled channel counting edges  0- count disable; 1-count rising edges; 2 - count falling edges; 3 - count both edges\n",
		"\tUsage:		plcpi <stack> optedgewr <channel> <edges> \n", "",
		"\tExample:	plcpi 0 optedgewr 2 1; Set Optocoupled channel #2 on Board #0 to count rising edges\n"};

const CliCmdType CMD_OPTO_EDGE_READ =
	{"optedgerd", 2, &doOptoEdgeRead,
		"\toptedgerd:	Read optocoupled counting edges 0 - none; 1 - rising; 2 - falling; 3 - both\n",
		"\tUsage:		plcpi <stack> optedgerd <pin>\n", "",
		"\tExample:		plcpi 0 optedgerd 2; Read counting edges of optocoupled channel #2 on Board #0\n"};

const CliCmdType CMD_OPTO_CNT_READ = {"optcntrd", 2, &doOptoCntRead,
	"\toptcntrd:	Read potocoupled inputs edges count for one pin\n",
	"\tUsage:		plcpi <stack> optcntrd <channel>\n", "",
	"\tExample:		plcpi 0 optcntrd 2; Read contor of opto input #2 on Board #0\n"};

const CliCmdType CMD_OPTO_CNT_RESET =
	{"optcntrst", 2, &doOptoCntReset,
		"\toptcntrst:	Reset optocoupled inputs edges count for one pin\n",
		"\tUsage:		plcpi <stack> optcntrst <channel>\n", "",
		"\tExample:		plcpi 0 optcntrst 2; Reset contor of opto input #2 on Board #0\n"};

const CliCmdType CMD_OPTO_ENC_WRITE =
	{"optencwr", 2, &doOptoEncoderWrite,
		"\toptencwr:	Enable / Disable optocoupled quadrature encoder, encoder 1 connected to opto ch1 and 2, encoder 2 on ch3 and 4 ... \n",
		"\tUsage:		plcpi <stack> optencwr <channel> <0/1> \n", "",
		"\tExample:	plcpi 0 optencwr 2 1; Enable encoder on opto channel 3/4  on Board stack level 0\n"};

const CliCmdType CMD_OPTO_ENC_READ =
	{"optencrd", 2, &doOptoEncoderRead,
		"\toptencrd:	Read optocoupled quadrature encoder state 0- disabled 1 - enabled\n",
		"\tUsage:		plcpi <stack> optencrd <channel>\n", "",
		"\tExample:		plcpi 0 optencrd 2; Read state of optocoupled encoder channel #2 on Board #0\n"};

const CliCmdType CMD_OPTO_ENC_CNT_READ =
	{"optcntencrd", 2, &doOptoEncoderCntRead,
		"\toptcntencrd:	Read potocoupled encoder count for one channel\n",
		"\tUsage:		plcpi <stack> optcntencrd <channel>\n", "",
		"\tExample:		plcpi 0 optcntencrd 2; Read contor of opto encoder #2 on Board #0\n"};

const CliCmdType CMD_OPTO_ENC_CNT_RESET =
	{"optcntencrst", 2, &doOptoEncoderCntReset,
		"\toptcntencrst:	Reset optocoupled encoder count \n",
		"\tUsage:		plcpi <stack> optcntencrst <channel>\n", "",
		"\tExample:		plcpi 0 optcntencrst 2; Reset contor of encoder #2 on Board #0\n"};

int odGet(int dev, int ch, float *val)
{
	u16 raw = 0;

	if ( (ch < CHANNEL_NR_MIN) || (ch > OD_CH_NR_MAX))
	{
		printf("Open drain channel out of range!\n");
		return ERROR;
	}
	if (OK
		!= i2cReadWordAS(dev, I2C_MEM_OD_PWM_VAL_RAW_ADD + 2 * (ch - 1), &raw))
	{
		printf("Fail to read!\n");
		return ERROR;
	}
	*val = 100 * (float)raw / OD_PWM_VAL_MAX;
	return OK;
}

int odSet(int dev, int ch, float val)
{
	u8 buff[2] = {0, 0};
	u16 raw = 0;

	if ( (ch < CHANNEL_NR_MIN) || (ch > OD_CH_NR_MAX))
	{
		printf("Open drain channel out of range!\n");
		return ERROR;
	}
	if (val < 0)
	{
		val = 0;
	}
	if (val > 100)
	{
		val = 100;
	}
	raw = (u16)ceil(OD_PWM_VAL_MAX * val / 100);
	memcpy(buff, &raw, 2);
	if (OK
		!= i2cMem8Write(dev, I2C_MEM_OD_PWM_VAL_RAW_ADD + 2 * (ch - 1), buff, 2))
	{
		printf("Fail to write!\n");
		return ERROR;
	}
	return OK;
}

int doOdRead(int argc, char *argv[]);
const CliCmdType CMD_OD_READ =
	{"odrd", 2, &doOdRead,
		"\todrd:		Read open drain output pwm value (0% - 100%)\n",
		"\tUsage:		plcpi <stack> odrd <channel>\n", "",
		"\tExample:		plcpi 0 odrd 2; Read pwm value of open drain channel #2 on Board #0\n"};

int doOdRead(int argc, char *argv[])
{
	int ch = 0;
	float val = 0;
	int dev = 0;

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}

	if (argc == 4)
	{
		ch = atoi(argv[3]);
		if ( (ch < CHANNEL_NR_MIN) || (ch > OD_CH_NR_MAX))
		{
			printf("Open drain channel out of range!\n");
			return (FAIL);
		}

		if (OK != odGet(dev, ch, &val))
		{
			printf("Fail to read!\n");
			return (FAIL);
		}

		printf("%0.2f\n", val);
	}
	else
	{
		printf("Invalid params number:\n %s", CMD_OD_READ.usage1);
		return (FAIL);
	}
	return OK;
}

int doOdWrite(int argc, char *argv[]);
const CliCmdType CMD_OD_WRITE =
	{"odwr", 2, &doOdWrite,
		"\todwr:		Write open drain output pwm value (0% - 100%), Warning: This function change the output of the coresponded DAC channel\n",
		"\tUsage:		plcpi <stack> odwr <channel> <value>\n", "",
		"\tExample:		plcpi 0 odwr 2 12.5; Write pwm 12.5% to open drain channel #2 on Board #0\n"};

int doOdWrite(int argc, char *argv[])
{
	int ch = 0;
	int dev = 0;
	float proc = 0;

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}

	if (argc == 5)
	{
		ch = atoi(argv[3]);
		if ( (ch < CHANNEL_NR_MIN) || (ch > OD_CH_NR_MAX))
		{
			printf("Open drain channel out of range!\n");
			return (FAIL);
		}
		proc = atof(argv[4]);
		if (proc < 0 || proc > 100)
		{
			printf("Invalid open drain pwm value, must be 0..100 \n");
			return (FAIL);
		}

		if (OK != odSet(dev, ch, proc))
		{
			printf("Fail to write!\n");
			return (FAIL);
		}
		printf("done\n");
	}
	else
	{
		printf("Invalid params number:\n %s", CMD_OD_WRITE.usage1);
		return (FAIL);
	}
	return OK;
}

//----------------------------------- OD pulses --------------------------------------------------------
#define SINGLE_TRANSFER

int odWritePulses(int dev, int ch, unsigned int val)
{
	u8 buff[5] = {0, 0, 0, 0, 0};
	u32 raw = 0;

	if ( (ch < CHANNEL_NR_MIN) || (ch > 2 * OD_CH_NR_MAX)) // channel from 5 to 8 are channel 1 to 4 in oposite direction
	{
		printf("Open drain channel out of range!\n");
		return ERROR;
	}
	raw = (u32)val;
	memcpy(buff, &raw, 4);

#ifdef SINGLE_TRANSFER
	buff[4] = ch;
	if (OK != i2cMem8Write(dev, I2C_MEM_OD_P_SET_VALUE, buff, 5)) // write the value
	{
		printf("Fail to write!\n");
		return ERROR;
	}

#else

	if (OK != i2cMem8Write(dev, I2C_MEM_OD_P_SET_VALUE, buff, 4)) // write the value
	{
		printf("Fail to write!\n");
		return ERROR;
	}
	buff[0] = ch;
	if (OK != i2cMem8Write(dev, I2C_MEM_OD_P_SET_CMD, buff, 1))// update command
	{
		printf("Fail to write!\n");
		return ERROR;
	}
#endif
	return OK;
}
#define PULSE_SAVE_MASK 0x10
#define PULSE_EXEC_MASK 0x20

int odSaveOdPulses(int dev, int ch, unsigned int val)
{
	u8 buff[5] = {0, 0, 0, 0, 0};
	u32 raw = 0;

	if ( (ch < CHANNEL_NR_MIN) || (ch > 2 * OD_CH_NR_MAX)) // channel from 5 to 8 are channel 1 to 4 in oposite direction
	{
		printf("Open drain channel out of range!\n");
		return ERROR;
	}
	raw = (u32)val;
	memcpy(buff, &raw, 4);

	buff[4] = ch | PULSE_SAVE_MASK;
	if (OK != i2cMem8Write(dev, I2C_MEM_OD_P_SET_VALUE, buff, 5)) // write the value
	{
		printf("Fail to write!\n");
		return ERROR;
	}

	return OK;
}

int odExecPulses(int dev, int ch) //execute previous saved poulses
{
	u8 buff[5] = {0, 0, 0, 0, 0};

	if ( (ch < CHANNEL_NR_MIN) || (ch > 2 * OD_CH_NR_MAX)) // channel from 5 to 8 are channel 1 to 4 in oposite direction
	{
		printf("Open drain channel out of range!\n");
		return ERROR;
	}
	buff[0] = (0x0f & (uint8_t)ch) | PULSE_EXEC_MASK;

	if (OK != i2cMem8Write(dev, I2C_MEM_OD_P_SET_CMD, buff, 1)) // write the value
	{
		printf("Fail to write!\n");
		return ERROR;
	}

	return OK;
}

int odResetPulses(int dev, int ch)
{
	return odWritePulses(dev, ch, 0);
}

int odReadPulses(int dev, int ch, unsigned int *val)
{
	u32 raw = 0;

	if ( (ch < CHANNEL_NR_MIN) || (ch > OD_CH_NR_MAX))
	{
		printf("Open drain channel out of range!\n");
		return ERROR;
	}
	if (OK != i2cReadDWord(dev, I2C_MEM_OD_PULSE_CNT_SET + 4 * (ch - 1), &raw))
	{
		printf("Fail to read!\n");
		return ERROR;
	}
	*val = raw;
	return OK;
}

int doOdCntRead(int argc, char *argv[]);
const CliCmdType CMD_OD_CNT_READ =
	{"odcrd", 2, &doOdCntRead,
		"\todcrd:		Read open drain remaining pulses to perform\n",
		"\tUsage:		plcpi <stack> odcrd <channel>\n", "",
		"\tExample:		plcpi 0 odcrd 2; Read remaining pulses to perform of open drain channel #2 on Board #0\n"};

int doOdCntRead(int argc, char *argv[])
{
	int ch = 0;
	unsigned int val = 0;
	int dev = 0;

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}
	if (argc == 4)
	{
		ch = atoi(argv[3]);
		if (OK != odReadPulses(dev, ch, &val))
		{
			return (FAIL);
		}
		printf("%d\n", val);
	}
	else
	{
		return ARG_CNT_ERR;
	}
	return OK;
}

int doOdCntWrite(int argc, char *argv[]);
const CliCmdType CMD_OD_CNT_WRITE =
	{"odcwr", 2, &doOdCntWrite,
		"\todcwr:			Write open drain output pulses to perform, value 0..65535. The open-drain channel will output <value> # of pulses 50% fill factor with current pwm frequency\n",
		"\tUsage:		plcpi <stack> odcwr <channel> <value>\n", "",
		"\tExample:		plcpi 0 odwr 2 100; set 100 pulses to perform for open drain channel #2 on Board #0\n"};

int doOdCntWrite(int argc, char *argv[])
{
	int ch = 0;
	int dev = 0;
	long int inVal = 0;
	unsigned int value = 0;

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}

	if (argc == 5)
	{
		ch = atoi(argv[3]);
		inVal = atol(argv[4]);
		value = (unsigned int)inVal;
		if (OK != odWritePulses(dev, ch, value))
		{
			return (FAIL);
		}
		printf("done\n");
	}
	else
	{
		printf("Invalid params number:\n %s", CMD_OD_CNT_WRITE.usage1);
		return (FAIL);
	}
	return OK;
}

int doOdCntSave(int argc, char *argv[]);
const CliCmdType CMD_OD_CNT_SAVE =
	{"odcs", 2, &doOdCntSave,
		"\todcs:			Save pulses counts to be executed with single byte command\n",
		"\tUsage:		plcpi <stack> odcs <channel> <value>\n", "",
		"\tExample:		plcpi 0 odcs 2 100; set 100 pulses to be performed for open drain channel #2 on Board #0\n"};

int doOdCntSave(int argc, char *argv[])
{
	int ch = 0;
	int dev = 0;
	long int inVal = 0;
	unsigned int value = 0;

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}

	if (argc == 5)
	{
		ch = atoi(argv[3]);
		inVal = atol(argv[4]);
		value = (unsigned int)inVal;
		if (OK != odSaveOdPulses(dev, ch, value))
		{
			return (FAIL);
		}
		printf("done\n");
	}
	else
	{
		printf("Invalid params number:\n %s", CMD_OD_CNT_SAVE.usage1);
		return (FAIL);
	}
	return OK;
}

int doOdCntExec(int argc, char *argv[]);
const CliCmdType CMD_OD_CNT_EXEC =
	{"odcx", 2, &doOdCntExec,
		"\todcx:			Execute previous saved pulses counts with single byte command\n",
		"\tUsage:		plcpi <stack> odcx <channel>\n", "",
		"\tExample:		plcpi 0 odcx 2 -> execute previous saved pulses for open drain channel #2 on Board #0\n"};

int doOdCntExec(int argc, char *argv[])
{
	int ch = 0;
	int dev = 0;

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}

	if (argc == 4)
	{
		ch = atoi(argv[3]);

		if (OK != odExecPulses(dev, ch))
		{
			return (FAIL);
		}
		printf("done\n");
	}
	else
	{
		printf("Invalid params number:\n %s", CMD_OD_CNT_EXEC.usage1);
		return (FAIL);
	}
	return OK;
}


int doOdCntReset(int argc, char *argv[]);
const CliCmdType CMD_OD_CNT_RST =
	{"odcrst", 2, &doOdCntReset,
		"\todcrst:			Reset open drain output pulses to perform\n",
		"\tUsage:		plcpi <stack> odcrst <channel>\n", "",
		"\tExample:		plcpi 0 odwr 2; stop pulses for open drain channel #2 on Board #0\n"};

int doOdCntReset(int argc, char *argv[])
{
	int ch = 0;
	int dev = 0;

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}

	if (argc == 4)
	{
		ch = atoi(argv[3]);

		if (OK != odResetPulses(dev, ch))
		{
			return (FAIL);
		}
		printf("done\n");
	}
	else
	{
		printf("Invalid params number:\n %s", CMD_OD_CNT_RST.usage1);
		return (FAIL);
	}
	return OK;
}

//*************************************************************************************

int pwmFreqGet(int dev, int *val)
{
	u16 raw = 0;

	if (OK != i2cReadWordAS(dev, I2C_MEM_OD_PWM_FREQUENCY, &raw))
	{
		printf("Fail to read!\n");
		return ERROR;
	}
	*val = raw;
	return OK;
}

int pwmFreqSet(int dev, int val)
{
	u8 buff[2] = {0, 0};
	u16 raw = 0;

	if (val < 10)
	{
		val = 10;
	}
	if (val > 65500)
	{
		val = 65500;
	}
	raw = (u16)val;
	memcpy(buff, &raw, 2);
	if (OK != i2cMem8Write(dev, I2C_MEM_OD_PWM_FREQUENCY, buff, 2))
	{
		printf("Fail to write!\n");
		return ERROR;
	}
	return OK;
}

int pwmChFreqSet(int dev, int ch, int val)
{
	u8 buff[2] = {0, 0};
	u16 raw = 0;

	if (val < 10)
	{
		val = 10;
	}
	if (val > 65500)
	{
		val = 65500;
	}
	raw = (u16)val;
	memcpy(buff, &raw, 2);
	if (OK
		!= i2cMem8Write(dev, I2C_MEM_OD_PWM_FREQUENCY_CH1 + (ch - 1) * 2, buff,
			2))
	{
		printf("Fail to write!\n");
		return ERROR;
	}
	return OK;
}

int doPwmFreqRead(int argc, char *argv[]);
const CliCmdType CMD_PWM_FREQ_READ =
	{"pwmfrd", 2, &doPwmFreqRead,
		"\tpwmfrd:		Read open-drain pwm frequency in Hz \n",
		"\tUsage:		plcpi <stack> pwmfrd\n", "",
		"\tExample:		plcpi 0 pwmfrd; Read the pwm frequency for all open drain output channels\n"};

int doPwmFreqRead(int argc, char *argv[])
{
	int val = 0;
	int dev = 0;

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}
	if (gHwVer < 3)
	{
		printf(
			"This feature is available on hardware versions greater or equal to 3.0!\n");
		return (FAIL);
	}
	if (argc == 3)
	{

		if (OK != pwmFreqGet(dev, &val))
		{
			printf("Fail to read!\n");
			return (FAIL);
		}

		printf("%d Hz\n", val);
	}
	else
	{
		printf("Invalid params number:\n %s", CMD_PWM_FREQ_READ.usage1);
		return (FAIL);
	}
	return OK;
}

int doPwmFreqWrite(int argc, char *argv[]);
const CliCmdType CMD_PWM_FREQ_WRITE =
	{"pwmfwr", 2, &doPwmFreqWrite,
		"\tpwmfwr:		Write open dran output pwm frequency in Hz [10..64000]\n",
		"\tUsage:		plcpi <stack> pwmfwr <value>\n",
		"\tUsage:		plcpi <stack> pwmfwr <channel> <value>\n",
		"\tExample:		plcpi 0 dacwr 200; Set the open-drain output pwm frequency to 200Hz \n"};

int doPwmFreqWrite(int argc, char *argv[])
{
	int dev = 0;
	int val = 0;
	int channel = 0;

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}
	if (gHwVer < 3)
	{
		printf(
			"This feature is available on hardware versions greater or equal to 3.0!\n");
		return (FAIL);
	}
	if (argc == 4)
	{
		val = atof(argv[3]);
		if (val < 10 || val > 65500)
		{
			printf("Invalid pwm frequency value, must be 10..65000 \n");
			return (FAIL);
		}

		if (OK != pwmFreqSet(dev, val))
		{
			printf("Fail to write!\n");
			return (FAIL);
		}
		printf("done\n");
	}
	else if (argc == 5)
	{
		channel = atoi(argv[3]);
		if (channel < 1 || channel > 4)
		{
			printf("Invalid channel number, must be 1..4 \n");
			return (FAIL);
		}
		val = atof(argv[4]);
		if (val < 10 || val > 65500)
		{
			printf("Invalid pwm frequency value, must be 10..65000 \n");
			return (FAIL);
		}

		if (OK != pwmChFreqSet(dev, channel, val))
		{
			printf("Fail to write!\n");
			return (FAIL);
		}
		printf("done\n");
	}
	else
	{
		printf("Invalid params number:\n %s", CMD_PWM_FREQ_WRITE.usage1);
		return (FAIL);
	}
	return OK;
}
#define MAX_ACC 60000
#define MAX_SPEED 60000
#define MIN_SPEED 10

int odOutMoveSet(int dev, int ch, int acc, int dec, int minSpd, int maxSpd)
{
	uint8_t buff[8];
	uint16_t aux16 = 0;

	if (ch <= 0 || ch > 4)
	{
		printf("invalid Channel number [1..4]\n");
		return -1;
	}
	if (acc < 0 || acc > MAX_ACC)
	{
		printf("Invalid acceleration value\n");
		return -1;
	}
	if (dec < 0 || dec > MAX_ACC)
	{
		printf("Invalid deceleration value\n");
		return -1;
	}
	if (maxSpd < MIN_SPEED || maxSpd > MAX_SPEED)
	{
		printf("Invalid speed [10..60000]\n");
	}

	if (minSpd < MIN_SPEED || minSpd > maxSpd)
	{
		printf("Invalid speed [10..60000]\n");
	}

	aux16 = (u16)acc;
	memcpy(buff, &aux16, sizeof(uint16_t));
	aux16 = (u16)dec;
	memcpy(buff + 2, &aux16, sizeof(uint16_t));
	aux16 = (u16)maxSpd;
	memcpy(buff + 4, &aux16, sizeof(uint16_t));
	aux16 = (u16)minSpd;
	memcpy(buff + 6, &aux16, sizeof(uint16_t));
	if (OK != i2cMem8Write(dev, I2C_MEM_ODP_ACC, buff, 8))
	{
		printf("Fail to write\n");
		return -1;
	}
	buff[0] = (uint8_t)ch;
	if (OK != i2cMem8Write(dev, I2C_MEM_ODP_CMD, buff, 1))
	{
		printf("Fail to write\n");
		return -1;
	}
	return OK;
}

int doMoveParWrite(int argc, char *argv[]);
const CliCmdType CMD_MV_P_WRITE =
	{"mvpwr", 2, &doMoveParWrite,
		"\tmvpwr:		Write open drain output movement profile parameters\n",
		"\tUsage:		plcpi <stack> mvpwr <channel> <acc> <dec> <min_speed> <max_speed>\n",
		"",
		"\tExample:		plcpi 0 mvpwr 1 1000 500 1000 20000; Set the open-drain output profile parameters \n"};

int doMoveParWrite(int argc, char *argv[])
{
	int dev = -1;
	int channel = 0;
	int acc = 0;
	int dec = 0;
	int maxSpd = 0;
	int minSpd = 0;

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}
	if (gHwVer < 3)
	{
		printf(
			"This feature is available on hardware versions greater or equal to 3.0!\n");
		return (FAIL);
	}

	if (argc != 8)
	{
		printf("Invalid argument number %s", CMD_MV_P_WRITE.usage1);
		return (FAIL);
	}
	channel = atoi(argv[3]);
	acc = atoi(argv[4]);
	dec = atoi(argv[5]);
	minSpd = atoi(argv[6]);
	maxSpd = atoi(argv[7]);

	return odOutMoveSet(dev, channel, acc, dec, minSpd, maxSpd);
}

//***************************************************Encoder threshold**********************************************
int encSetThreshold(int dev, int ch, unsigned int val)
{
	u8 buff[5] = {0, 0, 0, 0, 0};
	u32 raw = 0;

	if ( (ch < CHANNEL_NR_MIN) || (ch > OD_CH_NR_MAX)) //
	{
		printf("Open drain channel out of range!\n");
		return ERROR;
	}
	raw = (u32)val;
	memcpy(buff, &raw, 4);

	buff[4] = (uint8_t)ch;
	if (OK != i2cMem8Write(dev, I2C_MEM_ENCODER_LIMIT, buff, 5)) // write the value
	{
		printf("Fail to write!\n");
		return ERROR;
	}

	return OK;
}

int doEncThWr(int argc, char *argv[]);
const CliCmdType CMD_ENC_TH_WRITE =
	{"encthwr", 2, &doEncThWr,
		"\tencthwr:			Set the encoder threshold value and od channel action\n",
		"\tUsage:		plcpi <stack> encthwr <channel> <value>\n", "",
		"\tExample:		plcpi 0 encthwr 2 1000; set 1000 the threshold for encoder to reset open drain channel #2 pulses on Board #0\n"};

int doEncThWr(int argc, char *argv[])
{
	int ch = 0;
	int dev = 0;
	long int inVal = 0;
	unsigned int value = 0;

	dev = doBoardInit(atoi(argv[1]));
	if (dev <= 0)
	{
		return (FAIL);
	}

	if (argc == 5)
	{
		ch = atoi(argv[3]);
		inVal = atol(argv[4]);
		value = (unsigned int)inVal;
		if (OK != encSetThreshold(dev, ch, value))
		{
			return (FAIL);
		}
		printf("done\n");
	}
	else
	{
		printf("Invalid params number:\n %s", CMD_ENC_TH_WRITE.usage1);
		return (FAIL);
	}
	return OK;
}




const CliCmdType *gCmdArray[] = {&CMD_VERSION, &CMD_HELP, &CMD_WAR, &CMD_LIST,
	&CMD_BOARD,
#ifdef HW_DEBUG
	&CMD_ERR,
#endif
	&CMD_RELAY_WRITE, &CMD_RELAY_READ, &CMD_TEST, &CMD_GPIO_ENC_CNT_READ,
	&CMD_GPIO_ENC_CNT_RESET, &CMD_OPTO_READ, &CMD_OPTO_EDGE_READ,
	&CMD_OPTO_EDGE_WRITE, &CMD_OPTO_CNT_READ, &CMD_OPTO_CNT_RESET,
	&CMD_OPTO_ENC_WRITE, &CMD_OPTO_ENC_READ, &CMD_OPTO_ENC_CNT_READ,
	&CMD_OPTO_ENC_CNT_RESET, &CMD_OD_READ, &CMD_OD_WRITE, &CMD_OD_CNT_READ,
	&CMD_OD_CNT_WRITE, &CMD_OD_CNT_SAVE, &CMD_OD_CNT_EXEC, &CMD_OD_CNT_RST, &CMD_PWM_FREQ_READ, &CMD_PWM_FREQ_WRITE,
	&CMD_OPTO_OD_CMD_SET,
	&CMD_ENC_TH_WRITE,

	&CMD_MV_P_WRITE,

	NULL}; //null terminated array of cli structure pointers

int main(int argc, char *argv[])
{
	int i = 0;
	int ret = OK;

	if (argc == 1)
	{
		usage();
		return -1;
	}
#ifdef THREAD_SAFE
	sem_t *semaphore = sem_open("/SMI2C_SEM", O_CREAT);
	int semVal = 2;
	sem_wait(semaphore);
#endif
	while (NULL != gCmdArray[i])
	{
		if ( (gCmdArray[i]->name != NULL) && (gCmdArray[i]->namePos < argc))
		{
			if (strcasecmp(argv[gCmdArray[i]->namePos], gCmdArray[i]->name) == 0)
			{
				ret = gCmdArray[i]->pFunc(argc, argv);
				if (ret == ARG_CNT_ERR)
				{
					printf("Invalid parameters number!\n");
					printf("%s", gCmdArray[i]->usage1);
					if (strlen(gCmdArray[i]->usage2) > 2)
					{
						printf("%s", gCmdArray[i]->usage2);
					}
				}
#ifdef THREAD_SAFE
				sem_getvalue(semaphore, &semVal);
				if (semVal < 1)
				{
					sem_post(semaphore);
				}
#endif
				return ret;
			}
		}
		i++;
	}
	printf("Invalid command option\n");
	usage();
#ifdef THREAD_SAFE
	sem_getvalue(semaphore, &semVal);
	if (semVal < 1)
	{
		sem_post(semaphore);
	}
#endif
	return -1;
}
