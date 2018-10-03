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
* File : exo4.c
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

#define ROBOT_A_PRIO   		8				 // Defining Priority of each task
#define ROBOT_B_PRIO   		9
#define MUTEX_PRIO			6
#define CONTROLLER_PRIO     22

#define WORK_DATA_QUEUE_SIZE 10

/*
*********************************************************************************************************
*                                             VARIABLES
*********************************************************************************************************
*/

OS_STK           prepRobotAStk[TASK_STK_SIZE];	//Stack of each task
OS_STK           prepRobotBStk[TASK_STK_SIZE];
OS_STK           transportStk[TASK_STK_SIZE];
OS_STK           controllerStk[TASK_STK_SIZE];



OS_EVENT		*item_count;



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
*                                             STRUCTURES
*********************************************************************************************************
*/

typedef struct work_data {
	int work_data_a;
	int work_data_b;
} work_data;


OS_EVENT			*Fifo_Controller_to_robot_A;
work_data       FifoTbl[WORK_DATA_QUEUE_SIZE];

OS_EVENT			*Fifo_robot_A_to_robot_B;
work_data            	FifoTb2[WORK_DATA_QUEUE_SIZE];
/*
*********************************************************************************************************
*                                                  MAIN
*********************************************************************************************************
*/

void main(void)
{
	UBYTE err, errMutex;
	OSInit();
	// A completer

	err = OSTaskCreate(controller, (void*)0, &controllerStk[TASK_STK_SIZE - 1], CONTROLLER_PRIO);
	err |= OSTaskCreate(robotA, (void*)0, &prepRobotAStk[TASK_STK_SIZE - 1], ROBOT_A_PRIO);
	err |= OSTaskCreate(robotB, (void*)0, &prepRobotBStk[TASK_STK_SIZE - 1], ROBOT_B_PRIO);
	item_count = OSMutexCreate(MUTEX_PRIO, &errMutex);

	
	//transport ?
	Fifo_Controller_to_robot_A = OSQCreate(&FifoTbl[0], WORK_DATA_QUEUE_SIZE, sizeof(work_data));
	Fifo_robot_A_to_robot_B = OSQCreate(&FifoTb2[0], WORK_DATA_QUEUE_SIZE, sizeof(work_data));

	if (err != OS_ERR_NONE) {
		printf("Erreur lors de la création des tâches");
	}
	else if (Fifo_Controller_to_robot_A == NULL || Fifo_robot_A_to_robot_B == NULL) {
		printf("Erreur lors de la création des files");
	}

	else if (errMutex != OS_ERR_NONE) {
		printf("Erreur lors de la création du mutex");
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
	work_data* workData;

	printf("ROBOT A @ %d : DEBUT.\n", OSTimeGet() - startTime);

	while (1)
	{
		// A completer
		workData = OSQPend(Fifo_Controller_to_robot_A, 0, &err);
		errMsg(err, "Error while trying to access Fifo_Controller_to_robot_A");

		err = OSQPost(Fifo_robot_A_to_robot_B, workData->work_data_b);
		errMsg(err, "Error while trying to access Fifo_robot_A_to_robot_B");

		int counter = 0;
		while (counter < workData->work_data_a * 1000) { counter++; }
		printf("ROBOT A COMMANDE #%d avec %d items @ %d.\n", orderNumber, workData->work_data_a, OSTimeGet() - startTime);

		orderNumber++;

		OSMutexPend(item_count, 0, &err);
		errMsg(err, "Error while trying to access item_count");

		writeCurrentTotalItemCount(readCurrentTotalItemCount() + workData->work_data_a);
		err = OSMutexPost(item_count);
		errMsg(err, "Error while trying to access item_count");

	}
}

void robotB(void* data)
{
	INT8U err;
	int startTime = 0;
	int orderNumber = 1;
	printf("ROBOT B @ %d : DEBUT. \n", OSTimeGet() - startTime);
	int itemCountRobotB;
	while (1)
	{
		// A completer
		itemCountRobotB = OSQPend(Fifo_robot_A_to_robot_B, 0, &err);
		int counter = 0;
		while (counter < itemCountRobotB * 1000) { counter++; }
		printf("ROBOT B COMMANDE #%d avec %d items @ %d.\n", orderNumber, itemCountRobotB, OSTimeGet() - startTime);

		orderNumber++;

		OSMutexPend(item_count, 0, &err);
		errMsg(err, "Error while trying to access item_count");

		writeCurrentTotalItemCount(readCurrentTotalItemCount() + itemCountRobotB);
		err = OSMutexPost(item_count);
		errMsg(err, "Error while trying to access item_count");
	}
}

void controller(void* data)
{
	INT8U err;
	int startTime = 0;
	int randomTime = 0;
	work_data* workData;
	printf("TACHE CONTROLLER @ %d : DEBUT. \n", OSTimeGet() - startTime);

	for (int i = 1; i < 11; i++)
	{
		//Création d'une commande
		workData = malloc(sizeof(work_data));

		workData->work_data_a = (rand() % 8 + 3) * 10;
		workData->work_data_b = (rand() % 8 + 6) * 10;

		printf("TACHE CONTROLLER @ %d : COMMANDE #%d. \n prep time A = %d, prep time B = %d\n", OSTimeGet() - startTime, i, workData->work_data_a, workData->work_data_b);
		
		// A completer
		err = OSQPost(Fifo_Controller_to_robot_A, workData);
		errMsg(err, "Error while trying to access Fifo_Controller_to_robot_A");

		// Délai aléatoire avant nouvelle commande
		randomTime = (rand() % 9 + 5) * 4;
		OSTimeDly(randomTime);
	}

	free(workData);
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