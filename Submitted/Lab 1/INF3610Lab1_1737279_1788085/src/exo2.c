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
* File : exo2.c
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

#define ROBOT_A_PRIO   		9				 // Defining Priority of each task
#define ROBOT_B_PRIO   		8
#define CONTROLLER_PRIO     7
#define MUTEX_PRIO			6

/*
*********************************************************************************************************
*                                             VARIABLES
*********************************************************************************************************
*/

OS_STK           robotAStk[TASK_STK_SIZE];	//Stack of each task
OS_STK           robotBStk[TASK_STK_SIZE];
OS_STK           controllerStk[TASK_STK_SIZE];

/*
*********************************************************************************************************
*                                           SHARED  VARIABLES
*********************************************************************************************************
*/
OS_EVENT* sem_controller_to_robot_A;
OS_EVENT* robot_A_to_robotB;
OS_EVENT* item_count;

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
int		readCurrentTotalCount();
void	writeCurrentTotalCount(int qty);

/*
*********************************************************************************************************
*                                                  MAIN
*********************************************************************************************************
*/

void main(void)
{
	// A completer
	OSInit();

	INT8U err, errMutex;
	err = OSTaskCreate(controller, (void*)0, &controllerStk[TASK_STK_SIZE - 1], CONTROLLER_PRIO);
	err |= OSTaskCreate(robotA, (void*)0, &robotAStk[TASK_STK_SIZE - 1], ROBOT_A_PRIO);
	err |= OSTaskCreate(robotB, (void*)0, &robotBStk[TASK_STK_SIZE - 1], ROBOT_B_PRIO);
	sem_controller_to_robot_A = OSSemCreate(0);
	robot_A_to_robotB = OSSemCreate(0);
	item_count = OSMutexCreate(MUTEX_PRIO, &errMutex);

	if (err != OS_ERR_NONE ) {
		errMsg(err, "Erreur lors de la création des tâches");
	}
	else if (sem_controller_to_robot_A == NULL || robot_A_to_robotB == NULL) {
		printf("Erreur lors de la création des sémaphores");
	}

	else if (errMutex != OS_ERR_NONE) {
		errMsg(errMutex, "Erreur lors de la création du mutex");
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
		OSSemPend(sem_controller_to_robot_A, 0, &err);
		errMsg(err, "Error while trying to access sem_controller_to_robot_A");

		err = OSSemPost(robot_A_to_robotB);
		errMsg(err, "Error while trying to access robot_A_to_robot_B");

		// A completer

		int counter = 0;
		while (counter < itemCount * 1000) { counter++; }
		printf("ROBOT A COMMANDE #%d avec %d items @ %d.\n", orderNumber, itemCount, OSTimeGet() - startTime);
		orderNumber++;
		
		OSMutexPend(item_count, 0, &err);
		errMsg(err, "Error while trying to access item_count");

		writeCurrentTotalCount(readCurrentTotalCount() + itemCount);
		err = OSMutexPost(item_count);
		errMsg(err, "Error while trying to access item_count");



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
		OSSemPend(robot_A_to_robotB, 0, &err);
		errMsg(err, "Error while trying to access robot_A_to_robotB");

		int counter = 0;
		while (counter < itemCount * 1000) { counter++; };
		errMsg(err, "Error while trying to access item_count");
		printf("ROBOT B COMMANDE #%d avec %d items @ %d.\n", orderNumber, itemCount, OSTimeGet() - startTime);

		orderNumber++;

		OSMutexPend(item_count, 0, &err);
		errMsg(err, "Error while trying to access item_count");

		writeCurrentTotalCount(readCurrentTotalCount() + itemCount);
		err = OSMutexPost(item_count);
		errMsg(err, "Error while trying to access item_count");
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

		err = OSSemPost(sem_controller_to_robot_A);
		errMsg(err, "Error while trying to post sem_controller_to_robot_A");
	}
}

int	readCurrentTotalCount()
{
	OSTimeDly(2);
	return total_item_count;
}
void writeCurrentTotalCount(int newCount)
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