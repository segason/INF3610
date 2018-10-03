#include <ucos_ii.h>
#include <stdlib.h>
#include <inttypes.h>

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#define TASK_STK_SIZE 8192

/* ************************************************
 *                TASK PRIOS
 **************************************************/

#define          TASK_GENERATE_PRIO        10
#define 		 TASK_VERIF_SRC_PRIO       7
#define 		 TASK_VERIF_CRC_PRIO       8
#define			 TASK_STATS_PRIO		   9
#define          TASK_COMPUTING_PRIO       14
#define          TASK_FORWARDING_PRIO      16

#define          TASK_PRINT1_PRIO          11
#define          TASK_PRINT2_PRIO          12
#define          TASK_PRINT3_PRIO          13

#define			 MUT_TREATED_PRIO		   6
#define          MUT_PRINT_PRIO            5
#define          MUT_REJET_PRIO            4
#define          MUT_MALLOC_PRIO           3
#define          MUT_CRC_PRIO              2

// Routing info.
#define INT1_LOW      0x00000000
#define INT1_HIGH     0x3FFFFFFF
#define INT2_LOW      0x40000000
#define INT2_HIGH     0x7FFFFFFF
#define INT3_LOW      0x80000000
#define INT3_HIGH     0xBFFFFFFF
#define INT_BC_LOW    0xC0000000
#define INT_BC_HIGH   0xFFFFFFFF

// Reject source info.
#define REJECT_LOW1   0x10000000
#define REJECT_HIGH1  0x17FFFFFF
#define REJECT_LOW2   0x50000000
#define REJECT_HIGH2  0x57FFFFFF
#define REJECT_LOW3   0x60000000
#define REJECT_HIGH3  0x67FFFFFF
#define REJECT_LOW4   0xD0000000
#define REJECT_HIGH4  0xD7FFFFFF

typedef enum {
	PACKET_VIDEO, PACKET_AUDIO, PACKET_AUTRE, NB_PACKET_TYPE
} PACKET_TYPE;

typedef struct {
	unsigned int src;
	unsigned int dst;
	PACKET_TYPE type;
	unsigned int crc;
	unsigned int data[12];
} Packet;

typedef struct {
	unsigned int interfaceID;
	OS_EVENT *Mbox;
} PRINT_PARAM;

PRINT_PARAM print_param[3];

/* ************************************************
 *                  Mailbox
 **************************************************/

OS_EVENT *mbox[3];

/* ************************************************
 *                  Queues
 **************************************************/

OS_EVENT *inputQ;
OS_EVENT *lowQ;
OS_EVENT *mediumQ;
OS_EVENT *highQ;

/* ************************************************
 *                  Semaphores
 **************************************************/

/* � compl�ter */
OS_EVENT* semVerifySrc;
OS_EVENT* semVerifyCRC;
OS_EVENT* semStats;

/* ************************************************
 *                  Mutexes
 **************************************************/

/* � compl�ter */

OS_EVENT* mutexRejectedSrc;
OS_EVENT* mutexRejectedCRC;
OS_EVENT* mutexPrint;
OS_EVENT* mutexMalloc;
OS_EVENT* mutexTreatedPackets;

/* ************************************************
 *            Variables pour statistiques
 **************************************************/
OS_Q_DATA qData;

int mode_profilage; // Indique si nous sommes dans la p�riode de profilage ou non

int nb_echantillons; // Nb. d'�chantillons de la p�riode de profilage

int max_msg_input = 0; // Maximum de nombre d'items dans la file atteint durant
int max_msg_low = 0;	// la p�riode de profilage
int max_msg_medium = 0;
int max_msg_high = 0;

int moyenne_msg_input = 0; // Moyenne d'items dans la file atteint durant
int moyenne_msg_low = 0;	// la p�riode de profilage
int moyenne_msg_medium = 0;
int moyenne_msg_high = 0;

int nAccessInput = 0;
int nAccessHigh = 0;
int nAccessMedium = 0;
int nAccessLow = 0;

int nbPacketCrees = 0; // Nb de packets total créés
int nbPacketTraites = 0;	// Nb de paquets envoyés sur une interface
int nbPacketSourceRejete = 0; // Nb de packets rejetés pour mauvaise source
int nbPacketCRCRejete = 0; // Nb de packets rejetés pour mauvais CRC

/* ************************************************
 *              TASK PROTOTYPES
 **************************************************/

void TaskGeneratePacket(void *data); // Function prototypes of tasks
void TaskStats(void *data);
void TaskVerifySource(void *data);
void TaskVerifyCRC(void *data);
void TaskComputing(void *data);
void TaskForwarding(void *data);
void TaskPrint(void *data);

void create_application();
int create_tasks();
int create_events();
void err_msg(char*, uint8_t);
