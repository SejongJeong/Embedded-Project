#include "includes.h"
#include <avr/io.h>
#include <util/delay.h>

/*
*********************************************************************************************************
*                                               CONSTANTS
*********************************************************************************************************
*/

#define F_CPU 16000000UL // CPU frequency = 16 Mhz
#define TASK_STK_SIZE OS_TASK_DEF_STK_SIZE
#define MSG_QUEUE_SIZE 3

/*
*********************************************************************************************************
*                                               VARIABLES
*********************************************************************************************************
*/

static const unsigned char FND_DATA[10] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x27, 0x7f, 0x6f};
static const unsigned char fnd_se[4] = {0x01, 0x02, 0x04, 0x08};

OS_STK TaskStartStk[TASK_STK_SIZE];
OS_STK ProcessTaskStk[TASK_STK_SIZE];
OS_STK LedTaskStk[TASK_STK_SIZE];
OS_STK FndTaskStk[TASK_STK_SIZE];
OS_STK SwitchTaskStk[TASK_STK_SIZE];
OS_STK BuzzerTaskStk[TASK_STK_SIZE];
OS_STK CdsTaskStk[TASK_STK_SIZE];

/* Parameters to pass to each task */
INT8U ProcessTaskData;
INT8U LedTaskData;
INT8U FndTaskData;
INT8U SwitchTaskData;
INT8U BuzzerTaskData;
INT8U CdsTaskData;

// Semaphore
OS_EVENT *RandomSem;
OS_EVENT *SyncSem;

// Message Queue
OS_EVENT *MsgQueue_Random;

void *MsgQueue_Random_Table[MSG_QUEUE_SIZE];

/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void ProcessTask(void *data);
void LedTask(void *data);
void FndTask(void *data);
void SwitchTask(void *data);
void BuzzerTask(void *data);
void CdsTask(void *data);

void TaskStart(void *pdata); /* Function prototypes of Startup task           */
static void TaskStartCreateTasks(void);

void delay_ms(uint16_t count);
void delay_us(uint16_t count);

/*
*********************************************************************************************************
*                                          Embedded Board Function
*********************************************************************************************************
*/

void delay_ms(uint16_t count) {
	while(count--) {
		_delay_ms(1);

	}
}

void delay_us(uint16_t count) {
	while(count--) {
		_delay_us(1);

	}
}

/*
*********************************************************************************************************
*                                                MAIN
*********************************************************************************************************
*/

int main(void) {
    OSInit(); /* Initialize uC/OS-II */

    OS_ENTER_CRITICAL();
    TCCR0 = 0x07;
    TIMSK = _BV(TOIE0);
    TCNT0 = 256 - (CPU_CLOCK_HZ / OS_TICKS_PER_SEC / 1024);
    OS_EXIT_CRITICAL();

    PC_VectSet(uCOS, OSCtxSw); /* Install uC/OS-II's context switch vector */
    
    // Initiate sem
	RandomSem = OSSemCreate(1);
    SyncSem = OSSemCreate(0);

    OSTaskCreate(TaskStart, (void *)0, &TaskStartStk[TASK_STK_SIZE - 1], 0);

    OSStart(); /* Start multitasking  */

    return 0;
}

/*
*********************************************************************************************************
*                                              STARTUP TASK
*********************************************************************************************************
*/

void TaskStart(void *pdata) {
#if OS_CRITICAL_METHOD == 3 /* Allocate storage for CPU status register */
	OS_CPU_SR cpu_sr;
#endif
	char s[100];
	INT16S key;

	pdata = pdata; /* Prevent compiler warning                 */

	OS_ENTER_CRITICAL();
	PC_VectSet(0x08, OSTickISR);	  /* Install uC/OS-II's clock tick ISR        */
	PC_SetTickRate(OS_TICKS_PER_SEC); /* Reprogram tick rate                      */
	OS_EXIT_CRITICAL();

	OSStatInit(); /* Initialize uC/OS-II's statistics         */

	srand(time(0));

	TaskStartCreateTasks(); /* Create all the application tasks         */

	for (;;) {
		OSCtxSwCtr = 0;			   /* Clear context switch counter             */
		OSTimeDlyHMSM(0, 0, 1, 0); /* Wait one second                          */
	}
}

/*
*********************************************************************************************************
*                                             CREATE TASKS
*********************************************************************************************************
*/

static void TaskStartCreateTasks(void) {

    // Message Queue Create
    MsgQueue_Random = OSQCreate(&MsgQueue_Random_Table[0], 3);

	ProcessTaskData = 1;
	OSTaskCreate(ProcessTask, (void *)&ProcessTaskData, &ProcessTaskStk[TASK_STK_SIZE - 1], 1);

    FndTaskData = 2;
	OSTaskCreate(FndTask, (void *)&FndTaskData, &FndTaskStk[TASK_STK_SIZE - 1], 2);

}

/*
*********************************************************************************************************
*                                                  TASKS
*********************************************************************************************************
*/

void ProcessTask(void *data) {
    INT16U randNum1, randNum2;  // 암산할 Random Number
    INT16U op;  //  Operator
    INT8U err;

    data = data;

    while(1) {
        // randNum1,randNum2 9999 범위까지 Randum number 할당
        OSSemPend(RandomSem, 0, &err);
        randNum1 = random(10000);
        OSSemPost(RandomSem);

        OSSemPend(RandomSem, 0, &err);
        randNum2 = random(10000);
        OSSemPost(RandomSem);

        // op value 가 0이면 +, 1이면 -, 2이면 *, 3이면 /
        OSSemPend(RandomSem, 0, &err);
        op= random(4);
        OSSemPost(RandomSem);

        if (op == 0) { // op가 + 일 경우 3자리
            randNum1 %= 1000;
            randNum2 %= 1000;
        }
        else if (op == 2) { // op가 * 일 경우 2자리
            randNum1 %= 100;
            randNum2 %= 100;
        }

        OSQPost(MsgQueue_Random, (void *) &randNum1);
        OSQPost(MsgQueue_Random, (void *) &randNum2);
        OSQPost(MsgQueue_Random, (void *) &op);

        OSSemPend(SyncSem, 0, &err); // Wait 후 FndTask 실행

    }
}

void FndTask(void *data) {
    INT16U *num[3];
    INT8U err;

    data = data;

    for (i = 0; i < 4; i++)
    {
        num[3] = (INT16U *) OSQPend(MsgQueue_Random, 0, &err);
    }
}

void LedTask(void *data) {
    int i = 0;
    INT8U *getnumber[9];
    unsigned char value;

    data = data;

    DDRA = 0xff;
    while (1)
    {
        for (i = 0; i < 9; i++)
        {
            getnumber[i] = (INT8U *)OSQPend(MSGQueue, 0, &err);
        }
        for (i = 0; i < 9; i++)
        {
            value = *getnumber[i];
            PORTA = value;
            _delay_ms(200);
            value <<= 1;
        }
    }
}

void SwitchTask(void *data) {

    data = data;
}

void BuzzerTask(void *data) {

    data = data;
}

void CdsTask(void *data) {

    data = data;
}
