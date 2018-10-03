#include "routeur.h"
#include "bsp_init.h"
#include "platform.h"
#include <stdlib.h>
#include <stdbool.h>
#include <xil_printf.h>
#include <xgpio.h>

///////////////////////////////////////////////////////////////////////////////////////
//								uC-OS global variables
///////////////////////////////////////////////////////////////////////////////////////
#define TASK_STK_SIZE 8192

///////////////////////////////////////////////////////////////////////////////////////
//								Routines d'interruptions
///////////////////////////////////////////////////////////////////////////////////////

void timer_isr(void* not_valid) {
	if (private_timer_irq_triggered()) {
		private_timer_clear_irq();
		OSTimeTick();
	}
}

void fit_timer_1s_isr(void *not_valid) {
	/* � compl�ter */
	uint8_t err;
	err = OSSemPost(semVerifySrc);
	err_msg("Error while trying to access semVerifySrc", err);
}

void fit_timer_3s_isr(void *not_valid) {
	/* � compl�ter */
	uint8_t err;
	err = OSSemPost(semVerifyCRC);
	err_msg("Error while trying to access semVerifyCRC", err);
}

void gpio_isr(void * not_valid) {
	/* � compl�ter */
	uint8_t err;
	if (mode_profilage == 0) {
		nb_echantillons = nbPacketCrees;
		mode_profilage = 1;
	} else {
		err = OSSemPost(semStats);
		err_msg("Error while trying to access semStats", err);
		mode_profilage = 0;
	}
	XGpio_InterruptClear(&gpSwitch, 0xFFFFFFFF);

}

/*
 *********************************************************************************************************
 *                                            computeCRC
 * -Calcule la check value d'un pointeur quelconque (cyclic redudancy check)
 * -Retourne 0 si le CRC est correct, une autre valeur sinon.
 *********************************************************************************************************
 */
unsigned int computeCRC(uint16_t* w, int nleft) {
	unsigned int sum = 0;
	uint16_t answer = 0;

	// Adding words of 16 bits
	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}

	// Handling the last byte
	if (nleft == 1) {
		*(unsigned char *) (&answer) = *(const unsigned char *) w;
		sum += answer;
	}

	// Handling overflow
	sum = (sum & 0xffff) + (sum >> 16);
	sum += (sum >> 16);

	answer = ~sum;
	return (unsigned int) answer;
}

/*
 *********************************************************************************************************
 *                                          computePacketCRC
 * -Calcule la check value d'un paquet en utilisant un CRC (cyclic redudancy check)
 * -Retourne 0 si le CRC est correct, une autre valeur sinon.
 *********************************************************************************************************
 */
static inline unsigned int computePacketCRC(Packet* packet) {
	return computeCRC((uint16_t*) packet, sizeof(Packet));
}
///////////////////////////////////////////////////////////////////////////////////////
//								uC/OS-II part
///////////////////////////////////////////////////////////////////////////////////////
int main() {

	initialize_bsp();

	// Initialize uC/OS-II
	OSInit();

	create_application();

	prepare_and_enable_irq();

	xil_printf("*** Starting uC/OS-II scheduler ***\n");

	OSStart();

	mode_profilage = 0;

	cleanup();
	cleanup_platform();

	return 0;
}

void create_application() {
	int error;

	error = create_tasks();
	if (error != 0)
		xil_printf("Error %d while creating tasks\n", error);

	error = create_events();
	if (error != 0)
		xil_printf("Error %d while creating events\n", error);
}

int create_tasks() {
	// Stacks
	static OS_STK TaskReceiveStk[TASK_STK_SIZE];
	static OS_STK TaskVerifySourceStk[TASK_STK_SIZE];
	static OS_STK TaskVerifyCRCStk[TASK_STK_SIZE];
	static OS_STK TaskStatsStk[TASK_STK_SIZE];
	static OS_STK TaskComputeStk[TASK_STK_SIZE];
	static OS_STK TaskForwardingStk[TASK_STK_SIZE];
	static OS_STK TaskPrint1Stk[TASK_STK_SIZE];
	static OS_STK TaskPrint2Stk[TASK_STK_SIZE];
	static OS_STK TaskPrint3Stk[TASK_STK_SIZE];

	/* � compl�ter */
	uint8_t err;

	err = OSTaskCreate(TaskGeneratePacket, NULL,
			&TaskReceiveStk[TASK_STK_SIZE - 1], TASK_GENERATE_PRIO);
	err |= OSTaskCreate(TaskVerifySource, NULL,
			&TaskVerifySourceStk[TASK_STK_SIZE - 1], TASK_VERIF_SRC_PRIO);
	err |= OSTaskCreate(TaskVerifyCRC, NULL,
			&TaskVerifyCRCStk[TASK_STK_SIZE - 1], TASK_VERIF_CRC_PRIO);
	err |= OSTaskCreate(TaskComputing, NULL, &TaskComputeStk[TASK_STK_SIZE - 1],
			TASK_COMPUTING_PRIO);
	err |= OSTaskCreate(TaskForwarding, NULL,
			&TaskForwardingStk[TASK_STK_SIZE - 1], TASK_FORWARDING_PRIO);
	err |= OSTaskCreate(TaskStats, NULL, &TaskStatsStk[TASK_STK_SIZE - 1],
			TASK_STATS_PRIO);
	err |= OSTaskCreate(TaskPrint, &print_param[0],
			&TaskPrint1Stk[TASK_STK_SIZE - 1], TASK_PRINT1_PRIO);
	err |= OSTaskCreate(TaskPrint, &print_param[1],
			&TaskPrint2Stk[TASK_STK_SIZE - 1], TASK_PRINT2_PRIO);
	err |= OSTaskCreate(TaskPrint, &print_param[2],
			&TaskPrint3Stk[TASK_STK_SIZE - 1], TASK_PRINT3_PRIO);

	if (err != OS_ERR_NONE) {
		xil_printf("Error when trying to create tasks");
	}

	return 0;
}

int create_events() {
	uint8_t err;

	static void* inputMsg[1024];
	static void* lowMsg[1024];
	static void* mediumMsg[1024];
	static void* highMsg[1024];

	/* � compl�ter: cr�ation des files, mailbox, s�maphores et mutex */
	semVerifySrc = OSSemCreate(0);
	semVerifyCRC = OSSemCreate(0);
	semStats = OSSemCreate(0);

	mutexTreatedPackets = OSMutexCreate(MUT_TREATED_PRIO, &err);
	mutexRejectedSrc = OSMutexCreate(MUT_REJET_PRIO, &err);
	mutexRejectedCRC = OSMutexCreate(MUT_CRC_PRIO, &err);
	mutexPrint = OSMutexCreate(MUT_PRINT_PRIO, &err);
	mutexMalloc = OSMutexCreate(MUT_MALLOC_PRIO, &err);

	if (err != OS_ERR_NONE) {
		xil_printf("Error when trying to create mutexes");
	}

	for (uint8_t i = 0; i < 3; i++) {
		if (mbox[i] = OSMboxCreate(0)) {
			print_param[i].interfaceID = i + 1;
			print_param[i].Mbox = mbox[i];
		}
	}

	inputQ = OSQCreate(&inputMsg[0], 1024);
	lowQ = OSQCreate(&lowMsg[0], 1024);
	mediumQ = OSQCreate(&mediumMsg[0], 1024);
	highQ = OSQCreate(&highMsg[0], 1024);

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////
//								uC/OS-II part
///////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////
//									TASKS
///////////////////////////////////////////////////////////////////////////////////////

/*
 *********************************************************************************************************
 *											  TaskGeneratePacket
 *  - G�n�re des paquets et les envoie in la InputQ.
 *  - � des fins de d�veloppement de votre application, vous pouvez *temporairement* modifier la variable
 *    "shouldSlowthingsDown" � true pour ne g�n�rer que quelques paquets par seconde, et ainsi pouvoir
 *    d�boguer le flot de vos paquets de mani�re plus saine d'esprit. Cependant, la correction sera effectu�e
 *    avec cette variable à false.
 *********************************************************************************************************
 */
void TaskGeneratePacket(void *data) {
	srand(42);
	uint8_t err;
	bool isGenPhase = true; // Indique si on est in la phase de generation ou non
	const bool shouldSlowThingsDown = false;		// Variable à modifier
	int packGenQty = (rand() % 250);
	while (true) {
		if (isGenPhase) {
			OSMutexPend(mutexMalloc, 0, &err);
			err_msg(
					"error while trying to access mutexMalloc in TaskGeneratePacket",
					err);

			Packet *packet = malloc(sizeof(Packet));
			err = OSMutexPost(mutexMalloc);
			err_msg(
					"error while trying to access mutexMalloc in TaskGeneratePacket",
					err);

			packet->src = rand() * (UINT32_MAX / RAND_MAX);
			packet->dst = rand() * (UINT32_MAX / RAND_MAX);
			packet->type = rand() % NB_PACKET_TYPE;

			for (int i = 0; i < ARRAY_SIZE(packet->data); ++i)
				packet->data[i] = (unsigned int) rand();
			packet->data[0] = nbPacketCrees;

			//Compute CRC
			packet->crc = 0;
			if (rand() % 10 == 9) // 10% of Packets with bad CRC
				packet->crc = 1234;
			else
				packet->crc = computePacketCRC(packet);

			nbPacketCrees++;

			if (shouldSlowThingsDown) {
				OSMutexPend(mutexPrint, 0, &err);
				err_msg(
						"error while trying to access mutexPrint in TaskGeneratePacket \n",
						err);
				xil_printf(
						"GENERATE : ********GÃ©nÃ©ration du Paquet # %d ******** \n",
						nbPacketCrees);
				xil_printf("ADD %x \n", packet);
				xil_printf("	** src : %x \n", packet->src);
				xil_printf("	** dst : %x \n", packet->dst);
				xil_printf("	** crc : %x \n", packet->crc);
				xil_printf("	** type : %d \n", packet->type);
				err = OSMutexPost(mutexPrint);
				err_msg(
						"error while trying to access mutexPrint in TaskGeneratePacket \n",
						err);
			}

			err = OSQPost(inputQ, packet);

			if (err == OS_ERR_Q_FULL) {
				OSMutexPend(mutexPrint, 0, &err);
				err_msg(
						"error while trying to access mutexPrint in TaskGeneratePacket \n",
						err);
				xil_printf(
						"GENERATE: Paquet rejeté a l'entrée car la FIFO est pleine !\n");
				err = OSMutexPost(mutexPrint);
				err_msg(
						"error while trying to access mutexPrint in TaskGeneratePacket \n",
						err);
				OSMutexPend(mutexMalloc, 0, &err);
				err_msg(
						"error while trying to access mutexMalloc in TaskGeneratePacket",
						err);
				free(packet);
				err = OSMutexPost(mutexMalloc);
				err_msg(
						"error while trying to access mutexMalloc in TaskGeneratePacket",
						err);
			}

			if (shouldSlowThingsDown) {
				OSTimeDlyHMSM(0, 0, 0, 200 + rand() % 600);
			} else {
				OSTimeDlyHMSM(0, 0, 0, 2);

				if ((nbPacketCrees % packGenQty) == 0) //On génère jusqu'à 250 paquets par phase de génération
						{
					isGenPhase = false;
				}
			}
		} else {
			OSTimeDlyHMSM(0, 0, 0, 500);
			isGenPhase = true;
			packGenQty = (rand() % 250);
			OSMutexPend(mutexPrint, 0, &err);
			err_msg(
					"error while trying to access mutexPrint in TaskGeneratePacket \n",
					err);
			xil_printf(
					"GENERATE: Génération de %d paquets durant les %d prochaines millisecondes\n",
					packGenQty, packGenQty * 2);
			err = OSMutexPost(mutexPrint);
			err_msg(
					"error while trying to access mutexPrint in TaskGeneratePacket \n",
					err);
		}
	}
}

/*
 *********************************************************************************************************
 *											  TaskVerifySource
 *  -Stoppe le routeur une fois que 200 paquets ont �t� rejet�s pour mauvaise source
 *  -Ne doit pas stopper la t�che d'affichage des statistiques.
 *********************************************************************************************************
 */
void TaskVerifySource(void *data) {
	uint8_t err;
	while (true) {
		/* � compl�ter */
		OSSemPend(semVerifySrc, 0, &err);
		err_msg("Error while trying to access semVerifySrc in TaskVerifySource",
				err);
		OSMutexPend(mutexRejectedSrc, 0, &err);
		err_msg(
				"Error while trying to access mutexRejected in TaskVerifySource",
				err);
		if (nbPacketSourceRejete >= 200) {
			err = OSTaskSuspend(TASK_GENERATE_PRIO);
			err |= OSTaskSuspend(TASK_COMPUTING_PRIO);
			err |= OSTaskSuspend(TASK_FORWARDING_PRIO);
			err |= OSTaskSuspend(TASK_PRINT1_PRIO);
			err |= OSTaskSuspend(TASK_PRINT2_PRIO);
			err |= OSTaskSuspend(TASK_PRINT3_PRIO);
			if (err != OS_ERR_NONE) {
				err_msg(
						"Error while trying to suspend tasks in TaskVerifySource",
						err);
			}
		}
		err = OSMutexPost(mutexRejectedSrc);
		err_msg(
				"Error while trying to access mutexRejected in TaskVerifySource",
				err);
	}

}

/*
 *********************************************************************************************************
 *											  TaskVerifyCRC
 *  -Stoppe le routeur une fois que 200 paquets ont �t� rejet�s pour mauvais CRC
 *  -Ne doit pas stopper la t�che d'affichage des statistiques.
 *********************************************************************************************************
 */
void TaskVerifyCRC(void *data) {
	uint8_t err;
	while (true) {
		/* � compl�ter */
		OSSemPend(semVerifyCRC, 0, &err);
		err_msg("Error while trying to access semVerifyCRC in TaskVerifyCRC",
				err);
		OSMutexPend(mutexRejectedCRC, 0, &err);
		err_msg("Error while trying to access mutexRejected in TaskVerifyCRC",
				err);
		if (nbPacketCRCRejete >= 200) {
			err = OSTaskSuspend(TASK_GENERATE_PRIO);
			err = OSTaskSuspend(TASK_COMPUTING_PRIO);
			err |= OSTaskSuspend(TASK_FORWARDING_PRIO);
			err |= OSTaskSuspend(TASK_PRINT1_PRIO);
			err |= OSTaskSuspend(TASK_PRINT2_PRIO);
			err |= OSTaskSuspend(TASK_PRINT3_PRIO);
			if (err != OS_ERR_NONE) {
				xil_printf(
						"Error when trying to suspend tasks inTaskVerifyCRC");
			}
		}

		err = OSMutexPost(mutexRejectedCRC);
		err_msg("Error while trying to access mutexRejected in TaskVerifyCRC",
				err);
	}
}

void countMsgInputQ() {
	uint8_t err;
	if (mode_profilage) {
		err = OSQQuery(highQ, &qData);
		if (max_msg_input < qData.OSNMsgs)
			max_msg_input = qData.OSNMsgs;
		moyenne_msg_input += qData.OSNMsgs;
		nAccessInput++;
	}
}

void countMsgHighQ() {
	uint8_t err;
	if (mode_profilage) {
		err = OSQQuery(highQ, &qData);
		if (max_msg_high < qData.OSNMsgs)
			max_msg_high = qData.OSNMsgs;
		moyenne_msg_high += qData.OSNMsgs;
		nAccessHigh++;
	}
}

void countMsgMediumQ() {
	uint8_t err;
	if (mode_profilage) {
		err = OSQQuery(mediumQ, &qData);
		if (max_msg_medium < qData.OSNMsgs)
			max_msg_medium = qData.OSNMsgs;
		moyenne_msg_medium += qData.OSNMsgs;
		nAccessMedium++;
	}

}

void countMsgLowQ() {
	uint8_t err;
	if (mode_profilage) {
		err = OSQQuery(lowQ, &qData);
		if (max_msg_low < qData.OSNMsgs)
			max_msg_low = qData.OSNMsgs;
		moyenne_msg_low += qData.OSNMsgs;
		nAccessLow++;
	}

}

void TreatVideo(Packet * packet) {
	uint8_t err;
	countMsgHighQ();
	err = OSQPost(highQ, packet);
	err_msg("Error while trying to access highQ in TaskComputing", err);
	if (err == OS_ERR_Q_FULL) {
		OSMutexPend(mutexMalloc, 0, &err);
		err_msg("Error while trying to access mutexMalloc in TaskComputing",
				err);
		free(packet);
		err = OSMutexPost(mutexMalloc);
		err_msg("Error while trying to access mutexMalloc in TaskComputing",
				err);

		OSMutexPend(mutexPrint, 0, &err);
		err_msg("Error while trying to access mutexPrint in TaskComputing",
				err);
		xil_printf("HighQ is Full\n");
		err = OSMutexPost(mutexPrint);
		err_msg("Error while trying to access mutexPrint in TaskComputing",
				err);
	} else {
		OSMutexPend(mutexTreatedPackets, 0, &err);
		err_msg(
				"Error while trying to access mutexTreatedPackets in TaskComputing",
				err);
		nbPacketTraites++;
		err = OSMutexPost(mutexTreatedPackets);
		err_msg(
				"Error while trying to access mutexTreatedPackets in TaskComputing",
				err);

	}
}

void TreatAudio(Packet * packet) {
	uint8_t err;
	countMsgMediumQ();
	err = OSQPost(mediumQ, packet);
	err_msg("Error while trying to access mediumQ in TaskComputing", err);
	if (err == OS_ERR_Q_FULL) {
		OSMutexPend(mutexMalloc, 0, &err);
		err_msg("Error while trying to access mutexMalloc in TaskComputing",
				err);
		free(packet);
		err = OSMutexPost(mutexMalloc);
		err_msg("Error while trying to access mutexMalloc in TaskComputing",
				err);

		OSMutexPend(mutexPrint, 0, &err);
		err_msg("Error while trying to access mutexPrint in TaskComputing",
				err);
		xil_printf("mediumQ is Full\n");
		err = OSMutexPost(mutexPrint);
		err_msg("Error while trying to access mutexPrint in TaskComputing",
				err);
	} else {
		OSMutexPend(mutexTreatedPackets, 0, &err);
		err_msg(
				"Error while trying to access mutexTreatedPackets in TaskComputing",
				err);
		if (mode_profilage)
			nbPacketTraites++;
		err = OSMutexPost(mutexTreatedPackets);
		err_msg(
				"Error while trying to access mutexTreatedPackets in TaskComputing",
				err);

	}
}

void TreatAutre(Packet * packet) {
	uint8_t err;
	countMsgLowQ();
	err = OSQPost(lowQ, packet);
	err_msg("Error while trying to access lowQ in TaskComputing", err);
	if (err == OS_ERR_Q_FULL) {
		OSMutexPend(mutexMalloc, 0, &err);
		err_msg("Error while trying to access mutexMalloc in TaskComputing",
				err);
		free(packet);
		err = OSMutexPost(mutexMalloc);
		err_msg("Error while trying to access mutexMalloc in TaskComputing",
				err);

		OSMutexPend(mutexPrint, 0, &err);
		err_msg("Error while trying to access mutexPrint in TaskComputing",
				err);
		xil_printf("lowQ is Full\n");
		err = OSMutexPost(mutexPrint);
		err_msg("Error while trying to access mutexPrint in TaskComputing",
				err);
	} else {
		OSMutexPend(mutexTreatedPackets, 0, &err);
		err_msg(
				"Error while trying to access mutexTreatedPackets in TaskComputing",
				err);
		if (mode_profilage)
			nbPacketTraites++;
		err = OSMutexPost(mutexTreatedPackets);
		err_msg(
				"Error while trying to access mutexTreatedPackets in TaskComputing",
				err);

	}

}

void RejectPacketSrc(Packet* packet) {
	uint8_t err;
	OSMutexPend(mutexRejectedSrc, 0, &err);
	err_msg("Error while trying to access mutexRejectedSrc in TaskComputing",
			err);
	nbPacketSourceRejete++;
	err = OSMutexPost(mutexRejectedSrc);
	err_msg("Error while trying to access mutexRejectedSrc in TaskComputing",
			err);

	OSMutexPend(mutexMalloc, 0, &err);
	err_msg("Error while trying to access mutexMalloc in TaskComputing", err);
	free(packet);
	err = OSMutexPost(mutexMalloc);
	err_msg("Error while trying to access mutexMalloc in TaskComputing", err);

}

void RejectPacketCRC(Packet* packet) {
	uint8_t err;
	OSMutexPend(mutexRejectedCRC, 0, &err);
	err_msg("Error while trying to access mutexRejectedCRC in TaskComputing",
			err);
	nbPacketCRCRejete++;
	err = OSMutexPost(mutexRejectedCRC);
	err_msg("Error while trying to access mutexRejectedCRC in TaskComputing",
			err);

	OSMutexPend(mutexMalloc, 0, &err);
	err_msg("Error while trying to access mutexMalloc in TaskComputing", err);
	free(packet);
	err = OSMutexPost(mutexMalloc);
	err_msg("Error while trying to access mutexMalloc in TaskComputing", err);
}
/*
 *********************************************************************************************************
 *											  TaskComputing
 *  -V�rifie si les paquets sont conformes (CRC,Adresse Source)
 *  -Dispatche les paquets in des files (HIGH,MEDIUM,LOW)
 *
 *********************************************************************************************************
 */
void TaskComputing(void *pdata) {
	uint8_t err;
	Packet *packet = NULL;
	int busyWaitCnt = 220000;
	INT32U begin, end, clk;
	while (true) {
		/* � compl�ter */
		begin = OSTimeGet();
		err = OSQQuery(inputQ, &qData);
		countMsgInputQ();
		packet = OSQPend(inputQ, 0, &err);
		err_msg("Error while trying to access inputQ in TaskComputing", err);

		while (busyWaitCnt--)
			;
		busyWaitCnt = 220000;

		if ((packet->src >= REJECT_LOW1 && packet->src <= REJECT_HIGH1)
				| (packet->src >= REJECT_LOW2 && packet->src <= REJECT_HIGH2)
				| (packet->src >= REJECT_LOW3 && packet->src <= REJECT_HIGH3)
				| (packet->src >= REJECT_LOW4 && packet->src <= REJECT_HIGH4)) {
			RejectPacketSrc(packet);

		}

		else if (computePacketCRC(packet) != 0) {
			RejectPacketCRC(packet);

		} else if (packet->type == PACKET_VIDEO) {
			TreatVideo(packet);
		}

		else if (packet->type == PACKET_AUDIO) {
			TreatAudio(packet);

		}

		else if (packet->type == PACKET_AUTRE) {
			TreatAutre(packet);
		}

		end = OSTimeGet();
		clk = end - begin;
		OSMutexPend(mutexPrint, 0, &err);
		err_msg("Error while trying to access mutexPrint in TaskComputing",
				err);
		xil_printf("Temps de traitement d'un paquet: %d\n", clk);
		err = OSMutexPost(mutexPrint);
		err_msg("Error while trying to access mutexPrint in TaskComputing",
				err);
	}
}

/*
 *********************************************************************************************************
 *											  TaskForwarding
 *  -Traite la priorit� des paquets : si un paquet de haute priorit� est pr�t,
 *   on l'envoie � l'aide de la fonction dispatch, sinon on regarde les paquets de moins haute priorit�
 *********************************************************************************************************
 */
void TaskForwarding(void *pdata) {
	uint8_t err;
	Packet *packet = NULL;
	while (true) {
		/* � compl�ter */
		packet = OSQAccept(highQ, &err);
		if (err != OS_ERR_Q_EMPTY) {
			err_msg("Error while trying to access highQ in TaskForwarding",
					err);
		}

		if (packet == NULL) {
			packet = OSQAccept(mediumQ, &err);
			if (err != OS_ERR_Q_EMPTY) {
				err_msg(
						"Error while trying to access mediumQ in TaskForwarding",
						err);
			}
			if (packet == NULL) {
				packet = OSQAccept(lowQ, &err);
				if (err != OS_ERR_Q_EMPTY) {
					err_msg(
							"Error while trying to access lowQ in TaskForwarding",
							err);
				}
			}
		} else if (packet != NULL) {
			if (packet->dst >= INT1_LOW && packet->dst <= INT1_HIGH) {
				err = OSMboxPost(mbox[0], packet);
				err_msg("Error while trying to access INT1 in TaskForwarding",
						err);
			} else if (packet->dst >= INT2_LOW && packet->dst <= INT2_HIGH) {
				err = OSMboxPost(mbox[1], packet);
				err_msg("Error while trying to access INT2 in TaskForwarding",
						err);
			} else if (packet->dst >= INT3_LOW && packet->dst <= INT3_HIGH) {
				err = OSMboxPost(mbox[2], packet);
				err_msg("Error while trying to access INT3 in TaskForwarding",
						err);
			} else if (packet->dst >= INT_BC_LOW && packet->dst <= INT_BC_HIGH) {
				OSMutexPend(mutexMalloc, 0, &err);
				err_msg(
						"Error while trying to access mutexMalloc in TaskForwarding",
						err);
				Packet *secondPacket = malloc(sizeof(Packet));
				*secondPacket = *packet;
				Packet *thirdPacket = malloc(sizeof(Packet));
				*thirdPacket = *packet;

				err = OSMutexPost(mutexMalloc);
				err_msg(
						"Error while trying to access mutexMalloc in TaskForwarding",
						err);

				err = OSMboxPost(mbox[0], packet);
				err_msg("Error while trying to access BC in TaskForwarding",
						err);
				err = OSMboxPost(mbox[1], secondPacket);
				err_msg("Error while trying to access BC in TaskForwarding",
						err);
				err = OSMboxPost(mbox[2], thirdPacket);
				err_msg("Error while trying to access BC in TaskForwarding",
						err);

			}

		}
	}
}

/*
 *********************************************************************************************************
 *                                              TaskStats
 *  -Est d�clench�e lorsque le gpio_isr() lib�re le s�mpahore
 *  -Si en mode profilage, calcule les statistiques des files
 *  -En sortant de la p�riode de profilage, affiche les statistiques des files et du routeur.
 *********************************************************************************************************
 */
void TaskStats(void *pdata) {
	uint8_t err;
	while (true) {
		/* � compl�ter */

		if (mode_profilage == 0) {
			OSSemPend(semStats, 0, &err);
			err_msg("Error while trying to access semStats", err);

			OSMutexPend(mutexPrint, 0, &err);
			err_msg("Error while trying to access mutexPrint", err);

			xil_printf(
					"\n------------------ Affichage des statistiques ------------------\n");
			xil_printf("Nb de packets total crees : %d\n", nbPacketCrees);
			xil_printf("Nb de packets total traites : %d\n", nbPacketTraites);
			xil_printf("Nb de packets rejetes pour mauvaise source : %d\n",
					nbPacketSourceRejete);
			xil_printf("Nb de packets rejetes pour mauvais crc : %d\n",
					nbPacketCRCRejete);

			xil_printf("Nb d'echantillons de la p�riode de profilage : %d\n",
					nbPacketCrees - nb_echantillons);
			xil_printf("Maximum file input : %d\n", max_msg_input);
			xil_printf("Moyenne file input : %d\n",
					moyenne_msg_input / nAccessInput);
			xil_printf("Maximum file low : %d\n", max_msg_low);
			xil_printf("Moyenne file low : %d\n", moyenne_msg_low / nAccessLow);
			xil_printf("Maximum file medium : %d\n", max_msg_medium);
			xil_printf("Moyenne file medium : %d\n",
					moyenne_msg_medium / nAccessMedium);
			xil_printf("Maximum file high : %d\n", max_msg_high);
			xil_printf("Moyenne file high : %d\n",
					moyenne_msg_high / nAccessHigh);

			/* � compl�ter */
			resetValues();
			err = OSMutexPost(mutexPrint);
			err_msg("Error while trying to access mutexPrint", err);
		}
	}
}

void resetValues() {
	nb_echantillons = 0;

	max_msg_high = 0;
	moyenne_msg_high = 0;
	nAccessHigh = 0;

	max_msg_input = 0;
	moyenne_msg_input = 0;
	nAccessInput = 0;

	max_msg_low = 0;
	moyenne_msg_low = 0;
	nAccessLow = 0;

	max_msg_medium = 0;
	moyenne_msg_medium = 0;
	nAccessMedium = 0;

}

/*
 *********************************************************************************************************
 *											  TaskPrint
 *  -Affiche les infos des paquets arriv�s à destination et libere la m�moire allou�e
 *********************************************************************************************************
 */
void TaskPrint(void *data) {
	uint8_t err;
	Packet *packet = NULL;
	int intID = ((PRINT_PARAM*) data)->interfaceID;
	OS_EVENT* mb = ((PRINT_PARAM*) data)->Mbox;

	while (true) {
		/* � compl�ter */
		packet = OSMboxPend(mb, 0, &err);
		err_msg("Error while trying to access mb in TaskPrint", err);

		if (packet != NULL) {
			OSMutexPend(mutexPrint, 0, &err);
			err_msg("Error while trying to access mutexPrint  in TaskPrint",
					err);

			xil_printf("INT %d, SRC %08x, DST %08x, TYPE %d, DATA %x\n", intID,
					packet->src, packet->dst, packet->type, packet->data);
			err = OSMutexPost(mutexPrint);
			err_msg("Error while trying to access mutexPrint in TaskPrint",
					err);

			OSMutexPend(mutexMalloc, 0, &err);
			err_msg("Error while trying to access mutexMalloc in TaskPrint",
					err);
			free(packet);
			err = OSMutexPost(mutexMalloc);
			err_msg("Error while trying to access mutexMalloc in TaskPrint",
					err);
		} else {
			OSMutexPend(mutexPrint, 0, &err);
			err_msg("Error while trying to access mutexPrint in TaskPrint",
					err);
			xil_printf("Packet is NULL\n");
			err = OSMutexPost(mutexPrint);
			err_msg("Error while trying to access mutexPrint in TaskPrint",
					err);

		}
	}

}

void err_msg(char* entete, uint8_t err) {
	if (err != 0) {
		xil_printf(entete);
		xil_printf(": Une erreur est retourn�e  in err_Msg: code %d \n", err);
	}
}
