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

#define ROBOT_A1_PRIO   		8				 // Defining Priority of each task
#define ROBOT_B1_PRIO   		10
#define ROBOT_A2_PRIO   		9				 // Defining Priority of each task
#define ROBOT_B2_PRIO   		11

#define EQUIPE_1			1
#define EQUIPE_2			2


#define MUTEX_PRIO			6
#define SEM_EQUIPE_PRIO	23
#define MUTEX_DONE_PRIO		5
#define CONTROLLER_PRIO     22

#define WORK_DATA_QUEUE_SIZE 10

/*
*********************************************************************************************************
*                                             VARIABLES
*********************************************************************************************************
*/

OS_STK           prepRobotA1Stk[TASK_STK_SIZE];	//Stack of each task
OS_STK           prepRobotB1Stk[TASK_STK_SIZE];
OS_STK           prepRobotA2Stk[TASK_STK_SIZE];	//Stack of each task
OS_STK           prepRobotB2Stk[TASK_STK_SIZE];
OS_STK           controllerStk[TASK_STK_SIZE];

OS_EVENT		*item_count;
OS_EVENT		*sem_equipe;
OS_EVENT		*sem_equipeB;
OS_EVENT		*sem_full_robot;
OS_EVENT		*sem_full;
OS_EVENT		*mutex_done;




int controller_done = 0;


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

OS_EVENT			*Fifo_EQUIPE_1_robot_A_to_robot_B;
work_data            	FifoTb2[WORK_DATA_QUEUE_SIZE];

OS_EVENT			*Fifo_EQUIPE_2_robot_A_to_robot_B;
work_data            	FifoTb3[WORK_DATA_QUEUE_SIZE];
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
	err |= OSTaskCreate(robotA, EQUIPE_1, &prepRobotA1Stk[TASK_STK_SIZE - 1], ROBOT_A1_PRIO);
	err |= OSTaskCreate(robotB, EQUIPE_1, &prepRobotB1Stk[TASK_STK_SIZE - 1], ROBOT_B1_PRIO);

	err |= OSTaskCreate(robotA, EQUIPE_2, &prepRobotA2Stk[TASK_STK_SIZE - 1], ROBOT_A2_PRIO);
	err |= OSTaskCreate(robotB, EQUIPE_2, &prepRobotB2Stk[TASK_STK_SIZE - 1], ROBOT_B2_PRIO);
	item_count = OSMutexCreate(MUTEX_PRIO, &errMutex);
	sem_equipe = OSSemCreate(0);
	sem_equipeB = OSSemCreate(0);
	sem_full = OSSemCreate(0);
	sem_full_robot = OSSemCreate(0);
	mutex_done = OSMutexCreate(MUTEX_DONE_PRIO, &errMutex);

	Fifo_Controller_to_robot_A = OSQCreate(&FifoTbl[0], WORK_DATA_QUEUE_SIZE, sizeof(work_data));
	Fifo_EQUIPE_1_robot_A_to_robot_B = OSQCreate(&FifoTb2[0], WORK_DATA_QUEUE_SIZE, sizeof(work_data));
	Fifo_EQUIPE_2_robot_A_to_robot_B = OSQCreate(&FifoTb3[0], WORK_DATA_QUEUE_SIZE, sizeof(work_data));

	if (err != OS_ERR_NONE) {
		printf("Erreur lors de la création des tâches");
	}
	else if (Fifo_Controller_to_robot_A == NULL || Fifo_EQUIPE_1_robot_A_to_robot_B == NULL
		|| Fifo_EQUIPE_2_robot_A_to_robot_B == NULL) {
		printf("Erreur lors de la création des files");
	}

	else if (errMutex != OS_ERR_NONE) {
		printf("Erreur lors de la création du mutex");
	}
	else if (sem_equipe == NULL) {
		printf("Erreur lors de la création du sémaphore");
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
	int value;
	printf("ROBOT A%d @ %d : DEBUT.\n", data, OSTimeGet() - startTime);

	while (1)
	{
		// A completer
		OSMutexPend(mutex_done, 10, &err);
		errMsg(err, "Error while trying to access mutex_done");
		if (controller_done == 1) {
			err = OSMutexPost(mutex_done);
			errMsg(err, "Error while trying to access mutex_done");
			value = OSSemAccept(sem_full);


			if (value == NULL) {
				printf("Robot A no %d: va se suspendre\n",  data);
				OSTaskSuspend(OS_PRIO_SELF);
			}

			else {

				workData = OSQPend(Fifo_Controller_to_robot_A, 0, &err);
				errMsg(err, "Error while trying to access queue");

				if ((int) data == EQUIPE_1) {
					OSSemPend(sem_equipe, 0, &err);
					errMsg(err, "Error while trying to access sem_equipe");

					err = OSSemPost(sem_full_robot);
					errMsg(err, "Error while trying to access sem_full_robot");
					err = OSQPost(Fifo_EQUIPE_1_robot_A_to_robot_B, workData->work_data_b);
					errMsg(err, "Error while trying to access queue");
				}
				else {

					err = OSSemPost(sem_full_robot);
					errMsg(err, "Error while trying to access sem_full_robot");
					err = OSQPost(Fifo_EQUIPE_2_robot_A_to_robot_B, workData->work_data_b);
					errMsg(err, "Error while trying to access queue");
				}

				int counter = 0;
				while (counter < workData->work_data_a * 1000) { counter++; }
				printf("ROBOT A EQUIPE %d COMMANDE #%d avec %d items @ %d.\n", (int) data, orderNumber, workData->work_data_a, OSTimeGet() - startTime);

				orderNumber++;

				OSMutexPend(item_count, 0, &err);
				errMsg(err, "Error while trying to access item_count");
				writeCurrentTotalItemCount(readCurrentTotalItemCount() + workData->work_data_a);
				err = OSMutexPost(item_count);
				errMsg(err, "Error while trying to access item_count");

				if ((int) data == EQUIPE_2) {
					err = OSSemPost(sem_equipe);
					errMsg(err, "Error while trying to access sem_equipe");
				}

			}
		}
		else {

			err = OSMutexPost(mutex_done);
			errMsg(err, "Error while trying to access mutex_done");

			OSSemPend(sem_full, 200, &err);
			if (err != OS_ERR_TIMEOUT) {

				workData = OSQPend(Fifo_Controller_to_robot_A, 0, &err);
				errMsg(err, "Error while trying to access queue");

				if ((int) data == EQUIPE_1)
				{
					OSSemPend(sem_equipe, 0, &err);
					errMsg(err, "Error while trying to access sem_equipe");

					err = OSQPost(Fifo_EQUIPE_1_robot_A_to_robot_B, workData->work_data_b);
					err = OSSemPost(sem_full_robot);
					errMsg(err, "Error while trying to access sem_full_robot");
					errMsg(err, "Error while trying to access queue");
				}
				else {
					err = OSQPost(Fifo_EQUIPE_2_robot_A_to_robot_B, workData->work_data_b);
					err = OSSemPost(sem_full_robot);
					errMsg(err, "Error while trying to access sem_full_robot");
					errMsg(err, "Error while trying to access queue");
				}

				

				int counter = 0;
				while (counter < workData->work_data_a * 1000) { counter++; }
				printf("ROBOT A EQUIPE %d COMMANDE #%d avec %d items @ %d.\n", (int)data, orderNumber, workData->work_data_a, OSTimeGet() - startTime);

				orderNumber++;

				OSMutexPend(item_count, 0, &err);
				errMsg(err, "Error while trying to access item_count");

				writeCurrentTotalItemCount(readCurrentTotalItemCount() + workData->work_data_a);
				err = OSMutexPost(item_count);
				errMsg(err, "Error while trying to access item_count");

				if ((int) data == EQUIPE_2) {
					err = OSSemPost(sem_equipe);
					errMsg(err, "Error while trying to access sem_equipe");
				}
			}
		}

	}
}

void robotB(void* data)
{
	INT8U err;
	int startTime = 0;
	int orderNumber = 1;
	int value;
	printf("ROBOT B%d @ %d : DEBUT. \n", data, OSTimeGet() - startTime);
	int itemCountRobotB;
	while (1)
	{
		// A completer


		if (controller_done == 1) {

			value = OSSemAccept(sem_full_robot);
			if (value == NULL) {
				printf("Robot B no %d: va se suspendre\n", (int)data);
				OSTaskSuspend(OS_PRIO_SELF);
			}
			else {



				if ((int)data == EQUIPE_1) {
					OSSemPend(sem_equipeB, 0, &err);
					itemCountRobotB = OSQPend(Fifo_EQUIPE_1_robot_A_to_robot_B, 0, &err);
					errMsg(err, "Error while trying to access queue");
				}
				else {
					itemCountRobotB = OSQPend(Fifo_EQUIPE_2_robot_A_to_robot_B, 0, &err);
					errMsg(err, "Error while trying to access queue");
				}

				int counter = 0;
				while (counter < itemCountRobotB * 1000) { counter++; }
				printf("ROBOT B EQUIPE %d COMMANDE #%d avec %d items @ %d.\n", (int)data, orderNumber, itemCountRobotB, OSTimeGet() - startTime);

				orderNumber++;

				OSMutexPend(item_count, 0, &err);
				errMsg(err, "Error while trying to access item_count");
				writeCurrentTotalItemCount(readCurrentTotalItemCount() + itemCountRobotB);
				err = OSMutexPost(item_count);
				errMsg(err, "Error while trying to access item_count");

				if ((int)data == EQUIPE_2) {
					err = OSSemPost(sem_equipeB);
					errMsg(err, "Error while trying to access sem_equipe");
				}

			}
		}

		else {

			OSSemPend(sem_full_robot, 200, &err);

			if (err != OS_ERR_TIMEOUT) {
				if ((int)data == EQUIPE_1) {
					OSSemPend(sem_equipeB, 0, &err);
					itemCountRobotB = OSQPend(Fifo_EQUIPE_1_robot_A_to_robot_B, 0, &err);
					errMsg(err, "Error while trying to access queue");
				}
				else {
					itemCountRobotB = OSQPend(Fifo_EQUIPE_2_robot_A_to_robot_B, 0, &err);
					errMsg(err, "Error while trying to access queue");
				}

				int counter = 0;
				while (counter < itemCountRobotB * 1000) { counter++; }
				printf("ROBOT B EQUIPE %d COMMANDE #%d avec %d items @ %d.\n", (int)data, orderNumber, itemCountRobotB, OSTimeGet() - startTime);

				orderNumber++;

				OSMutexPend(item_count, 0, &err);
				errMsg(err, "Error while trying to access item_count");
				writeCurrentTotalItemCount(readCurrentTotalItemCount() + itemCountRobotB);
				err = OSMutexPost(item_count);
				errMsg(err, "Error while trying to access item_count");
			
				if ((int)data == EQUIPE_2) {
					err = OSSemPost(sem_equipeB);
					errMsg(err, "Error while trying to access sem_equipe");
				}
			}
		}
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
		err = OSSemPost(sem_full);
		errMsg(err, "Error while trying to access sem_full");
		// Délai aléatoire avant nouvelle commande
		randomTime = (rand() % 9 + 5) * 4;
		OSTimeDly(randomTime);
	}

	free(workData);
	OSMutexPend(mutex_done, 0, &err);
	errMsg(err, "Error while trying to access mutex_done");

	printf("Fin de la Production\n");
	controller_done = 1;
	
	err = OSMutexPost(mutex_done);
	errMsg(err, "Error while trying to access mutex_done");

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