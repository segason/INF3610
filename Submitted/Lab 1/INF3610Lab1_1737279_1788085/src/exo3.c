/*
*********************************************************************************************************
*                                                 uC/OS-II
*                                          The Real-Time Kernel
*                                               PORT Windows
*
*
*            		          	Arnaud Desaulty, Frederic Fortier, Eva Terriault
*                                  Ecole Polytechnique de Montreal, Qc, CANADA
*                                                  01/2018
*
* File : exo3.c
*
*********************************************************************************************************
*/

// Main include of µC-II
#include "includes.h"
/*
*********************************************************************************************************
*                                              CONSTANTS
*********************************************************************************************************
*/

#define TASK_STK_SIZE       16384            // Size of each task's stacks (# of WORDs)

#define ROBOT_A_PRIO   		10				 // Defining Priority of each task
#define ROBOT_B_PRIO   		9
#define CONTROLLER_PRIO     7
#define MUTEX_PRIO			6
#define FLAG_CONTROLLER_TO_ROBOT_A 0X01
#define FLAG_ROBOT_A_TO_ROBOT_B 0X02

/*
*********************************************************************************************************
*                                             VARIABLES
*********************************************************************************************************
*/

OS_STK           robotAStk[TASK_STK_SIZE];	//Stack of each task
OS_STK           robotBStk[TASK_STK_SIZE];
OS_STK           controllerStk[TASK_STK_SIZE];
OS_FLAG_GRP *FlagGroup;

OS_EVENT* item_count;

/*
*********************************************************************************************************
*                                           SHARED  VARIABLES
*********************************************************************************************************
*/

volatile int total_item_count = 0;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/
void    robotA(void *data);
void    robotB(void *data);
void    controller(void *data);
void    errMsg(INT8U err, char* errMSg);
int		readCurrentTotalItemCount();
void	writeCurrentTotalItemCount(int qty);

/*
*********************************************************************************************************
*                                                  MAIN
*********************************************************************************************************
*/

void main(void)
{
	UBYTE errTask, errFlags, err;

	OSInit();

	// A completer
	//Gestion des erreurs
	errTask = OSTaskCreate(controller, (void*)0, &controllerStk[TASK_STK_SIZE - 1], CONTROLLER_PRIO);
	errTask |= OSTaskCreate(robotA, (void*)0, &robotAStk[TASK_STK_SIZE - 1], ROBOT_A_PRIO);
	errTask |= OSTaskCreate(robotB, (void*)0, &robotBStk[TASK_STK_SIZE - 1], ROBOT_B_PRIO);
	FlagGroup = OSFlagCreate(0x00, &errFlags);
	item_count = OSMutexCreate(MUTEX_PRIO, &err);

	if (errTask != OS_ERR_NONE) {
		printf("Erreur lors de la création des tâches");
	}
	else if (errFlags != OS_ERR_NONE) {
		printf("Erreur lors de la création des flags");
	}
	if (err != OS_ERR_NONE) {
		printf("Erreur lors de la création des mutex");
	}
	else {
		OSStart();
	}

	return;
}

/*
*********************************************************************************************************
*                                            TASK FUNCTIONS
*********************************************************************************************************
*/

void robotA(void* data)
{
	INT8U err;
	int startTime = 0;
	int orderNumber = 1;
	int itemCount;
	printf("ROBOT A @ %d : DEBUT.\n", OSTimeGet() - startTime);
	while (1)
	{
		itemCount = (rand() % 7 + 1) * 10;


		//mettre à jour quantite totale
		// A completer
		OSFlagPend(FlagGroup, FLAG_CONTROLLER_TO_ROBOT_A, OS_FLAG_WAIT_SET_ANY, 0, &err);
		errMsg(err, "Error while trying to access FLAG_CONTROLLER_TO_ROBOT_A");

		OSFlagPost(FlagGroup, FLAG_ROBOT_A_TO_ROBOT_B, OS_FLAG_SET, &err);
		errMsg(err, "Error while trying to access FLAG_ROBOT_A_TO_ROBOT_B");



		int counter = 0;
		while (counter < itemCount * 1000) { counter++; }
		printf("ROBOT A COMMANDE #%d avec %d items @ %d.\n", orderNumber, itemCount, OSTimeGet() - startTime);

		orderNumber++;

		OSMutexPend(item_count, 0, &err);
		errMsg(err, "Error while trying to access item_count");

		writeCurrentTotalItemCount(readCurrentTotalItemCount() + itemCount);
		err = OSMutexPost(item_count);
		errMsg(err, "Error while trying to access item_count");

		OSFlagPost(FlagGroup, FLAG_CONTROLLER_TO_ROBOT_A, OS_FLAG_CLR, &err);
		errMsg(err, "Error while trying to access FLAG_CONTROLLER_TO_ROBOT_A");
	}
}

void robotB(void* data)
{
	INT8U err;
	int startTime = 0;
	int orderNumber = 1;
	int itemCount;
	printf("ROBOT B @ %d : DEBUT. \n", OSTimeGet() - startTime);
	while (1)
	{
		itemCount = (rand() % 6 + 2) * 10;

		// A completer
		OSFlagPend(FlagGroup, FLAG_ROBOT_A_TO_ROBOT_B, OS_FLAG_WAIT_SET_ANY, 0, &err);
		errMsg(err, "Error while trying to access FLAG_ROBOT_A_TO_ROBOT_B");

		int counter = 0;
		while (counter < itemCount * 1000) { counter++; }
		printf("ROBOT B COMMANDE #%d avec %d items @ %d.\n", orderNumber, itemCount, OSTimeGet() - startTime);

		orderNumber++;

		OSMutexPend(item_count, 0, &err);
		errMsg(err, "Error while trying to access item_count");

		writeCurrentTotalItemCount(readCurrentTotalItemCount() + itemCount);
		err = OSMutexPost(item_count);
		errMsg(err, "Error while trying to access item_count");

		OSFlagPost(FlagGroup, FLAG_ROBOT_A_TO_ROBOT_B, OS_FLAG_CLR, &err);
		errMsg(err, "Error while trying to access FLAG_ROBOT_A_TO_ROBOT_B");


	}
}

void controller(void* data)
{
	INT8U err;
	int startTime = 0;
	int randomTime = 0;
	printf("CONTROLLER @ %d : DEBUT. \n", OSTimeGet() - startTime);
	for (int i = 1; i < 11; i++)
	{
		randomTime = (rand() % 9 + 5) * 10;
		OSTimeDly(randomTime);

		printf("CONTROLLER @ %d : COMMANDE #%d. \n", OSTimeGet() - startTime, i);

		// A completer
		OSFlagPost(FlagGroup, FLAG_CONTROLLER_TO_ROBOT_A, OS_FLAG_SET, &err);
		errMsg(err, "error");
	}
}

int	readCurrentTotalItemCount()
{
	OSTimeDly(2);
	return total_item_count;
}
void writeCurrentTotalItemCount(int newCount)
{
	OSTimeDly(2);
	total_item_count = newCount;
}

void errMsg(INT8U err, char* errMsg)
{
	if (err != OS_ERR_NONE)
	{
		printf(errMsg);
		exit(1);
	}
}