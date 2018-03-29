/*
 * FreeRTOS Kernel V10.0.0
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software. If you wish to use our Amazon
 * FreeRTOS name, please do so in a fair use way that does not cause confusion.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/******************************************************************************
 * NOTE: Windows will not be running the FreeRTOS demo threads continuously, so
 * do not expect to get real time behaviour from the FreeRTOS Windows port, or
 * this demo application.  Also, the timing information in the FreeRTOS+Trace
 * logs have no meaningful units.  See the documentation page for the Windows
 * port for further information:
 * http://www.freertos.org/FreeRTOS-Windows-Simulator-Emulator-for-Visual-Studio-and-Eclipse-MingW.html
 *
 * NOTE 2:  This project provides two demo applications.  A simple blinky style
 * project, and a more comprehensive test and demo application.  The
 * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting in main.c is used to select
 * between the two.  See the notes on using mainCREATE_SIMPLE_BLINKY_DEMO_ONLY
 * in main.c.  This file implements the simply blinky version.  Console output
 * is used in place of the normal LED toggling.
 *
 * NOTE 3:  This file only contains the source code that is specific to the
 * basic demo.  Generic functions, such FreeRTOS hook functions, are defined
 * in main.c.
 ******************************************************************************
 *
 * main_blinky() creates one queue, one software timer, and two tasks.  It then
 * starts the scheduler.
 *
 * The Queue Send Task:
 * The queue send task is implemented by the prvQueueSendTask() function in
 * this file.  It uses vTaskDelayUntil() to create a periodic task that sends
 * the value 100 to the queue every 200 milliseconds (please read the notes
 * above regarding the accuracy of timing under Windows).
 *
 * The Queue Send Software Timer:
 * The timer is a one-shot timer that is reset by a key press.  The timer's
 * period is set to two seconds - if the timer expires then its callback
 * function writes the value 200 to the queue.  The callback function is
 * implemented by prvQueueSendTimerCallback() within this file.
 *
 * The Queue Receive Task:
 * The queue receive task is implemented by the prvQueueReceiveTask() function
 * in this file.  prvQueueReceiveTask() waits for data to arrive on the queue.
 * When data is received, the task checks the value of the data, then outputs a
 * message to indicate if the data came from the queue send task or the queue
 * send software timer.
 *
 * Expected Behaviour:
 * - The queue send task writes to the queue every 200ms, so every 200ms the
 *   queue receive task will output a message indicating that data was received
 *   on the queue from the queue send task.
 * - The queue send software timer has a period of two seconds, and is reset
 *   each time a key is pressed.  So if two seconds expire without a key being
 *   pressed then the queue receive task will output a message indicating that
 *   data was received on the queue from the queue send software timer.
 *
 * NOTE:  Console input and output relies on Windows system calls, which can
 * interfere with the execution of the FreeRTOS Windows port.  This demo only
 * uses Windows system call occasionally.  Heavier use of Windows system calls
 * can crash the port.
 */

/* Standard includes. */
#include <stdio.h>
#include <conio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

/* Priorities at which the tasks are created. */
#define tskReadPC_PRIORITY			( tskIDLE_PRIORITY + 1 )
#define tskController_PRIORITY		( tskIDLE_PRIORITY + 2 )
#define tskPrintOutput_PRIORITY		( tskIDLE_PRIORITY + 3 )

//Queue: User input definition. 
#define TIMER_5s	(1UL)
#define TIMER_10s	(2UL)
#define TIMER_15s	(3UL)
#define USER_START	(4UL)
#define USER_PAUSE	(5UL)
#define USER_RESUME (6UL)
#define USER_CANCEL (7UL)
#define POWER_ON	(8UL)
#define POWER_OFF	(9UL)
#define C_NORMAL	(10UL)
#define C_FAILURE	(11UL)
#define TIMER_EXPIRED (12UL)
#define DEFAULT_STATE (13UL)

//Notified Value for Print Output task
#define EM_ON		(1UL)
#define EM_OFF		(0UL)
#define LIGHT_ON	(1UL)
#define LIGHT_OFF	(0UL)
#define BUZZER_ON	(1UL)
#define BUZZER_OFF	(0UL)

/* The number of items the queue can hold at once. */
#define QUEUE_LENGTH	( 2 )

void ReadKeyBoard(void *pvParameters);
void Controller(void *pvParameters);
void PrintOutput(void *pvParameters);
void PrintUserOptions();

TimerHandle_t xTimers;
TaskHandle_t hPrintOutput = NULL;
QueueHandle_t xQueue       = NULL;

void vTimerCallback(TimerHandle_t xTimer)
{
	//When times up, callback function send TIMER_EXPIRED to controller via queue
	(void) xTimer;
	uint32_t ulQueueValue = TIMER_EXPIRED;
	xQueueSend(xQueue, &ulQueueValue, 0U);
}

void main_blinky(void)
{
	PrintUserOptions(); //To print available user options.

	xQueue = xQueueCreate(QUEUE_LENGTH, sizeof(uint32_t));

	xTaskCreate(ReadKeyBoard, "RD_KB", configMINIMAL_STACK_SIZE, NULL, tskReadPC_PRIORITY		, NULL);
	xTaskCreate(Controller	, "CTR"	 , configMINIMAL_STACK_SIZE, NULL, tskController_PRIORITY	, NULL);
	xTaskCreate(PrintOutput	, "PO"	 , configMINIMAL_STACK_SIZE, NULL, tskPrintOutput_PRIORITY	, &hPrintOutput);
	
	xTimers = xTimerCreate ("Timer", pdMS_TO_TICKS(2000UL), pdFALSE, NULL, vTimerCallback);

	vTaskStartScheduler();

	/* Should not reach here. */
	for (;; );
}

void ReadKeyBoard(void *pvParameters)
{
	(void)pvParameters;
	TickType_t xNextWakeTime = xTaskGetTickCount(); 
	uint32_t keyboardValue;
	uint32_t ulQueueValue;

	for (;;)
	{
		if (_kbhit() != 0)
		{
			ulQueueValue = 0;
			keyboardValue = _getch();
			switch (keyboardValue)
			{
				case '1': ulQueueValue = TIMER_5s; break;
				case '2': ulQueueValue = TIMER_10s; break;
				case '3': ulQueueValue = TIMER_15s; break;
				case 'p': ulQueueValue = USER_PAUSE; break;
				case 'r': ulQueueValue = USER_RESUME; break;
				case 'c': ulQueueValue = USER_CANCEL; break;
				case '8':
					ulQueueValue = POWER_ON;
					printf("Power is ON...\n");
					break;
				case '9':
					ulQueueValue = POWER_OFF;
					printf("Power is OFF...\n");
					break;
				case 'n':
					ulQueueValue = C_NORMAL;
					//printf("Condition Normal...\n");
					break;
				case 'f':
					ulQueueValue = C_FAILURE;
					//printf("Condition Fail...\n");
					break;
			}

			if (ulQueueValue != 0) 
			{ 
				xQueueSend(xQueue, &ulQueueValue, 0U);
			}
		}

		//Detect keyboard input every 0.1s
		vTaskDelayUntil(&xNextWakeTime, pdMS_TO_TICKS(100UL));
	}
}

void Controller(void *pvParameters)
{
	(void)pvParameters;
	uint32_t ulReceivedValue = 0;
	uint32_t ulRemainingTick = 0;
	uint32_t ulNotifiedValue;
	uint32_t ulCond = POWER_OFF;

	for (;;)
	{
		ulNotifiedValue = 0;
		xQueueReceive(xQueue, &ulReceivedValue, portMAX_DELAY);

		switch (ulCond)
		{
			case POWER_OFF:
				switch (ulReceivedValue)
				{
					case POWER_ON:
						ulCond = C_NORMAL;
						ulNotifiedValue = (EM_OFF << 0) | (LIGHT_OFF << 1) | (BUZZER_OFF << 2) | (C_NORMAL << 3);
						break;
				}
				break;

			case POWER_ON:
			case C_NORMAL:
				switch (ulReceivedValue)
				{
					case TIMER_5s:
						xTimerChangePeriod(xTimers, pdMS_TO_TICKS(5000UL), 200);
						ulNotifiedValue = (EM_ON << 0) | (LIGHT_ON << 1) | (BUZZER_OFF << 2) | (TIMER_5s << 3);
						printf("5s Timer starts...\n");
						break;
					case TIMER_10s:
						xTimerChangePeriod(xTimers, pdMS_TO_TICKS(10000UL), 200);
						ulNotifiedValue = (EM_ON << 0) | (LIGHT_ON << 1) | (BUZZER_OFF << 2) | (TIMER_10s << 3);
						printf("10s Timer starts...\n");
						break;
					case TIMER_15s:
						xTimerChangePeriod(xTimers, pdMS_TO_TICKS(15000UL), 200);
						ulNotifiedValue = (EM_ON << 0) | (LIGHT_ON << 1) | (BUZZER_OFF << 2) | (TIMER_15s << 3);
						printf("15s Timer starts...\n");
						break;
					case USER_PAUSE:
						xTimerStop(xTimers, 200);
						ulRemainingTick = xTimerGetExpiryTime(xTimers) - xTaskGetTickCount();
						ulNotifiedValue = (EM_OFF << 0) | (LIGHT_OFF << 1) | (BUZZER_OFF << 2) | (USER_PAUSE << 3);
						printf("Microwave Paused...\n");
						break;
					case USER_RESUME:
						printf("Microwave Resumed...\n");
						if (ulRemainingTick != 0)
						{
							xTimerChangePeriod(xTimers, ulRemainingTick, 200);
							ulNotifiedValue = (EM_ON << 0) | (LIGHT_ON << 1) | (BUZZER_OFF << 2) | (USER_RESUME << 3);
						}
						else
						{
							printf("No remaining time left...\n");
						}
						break;
					case USER_CANCEL:
						xTimerStop(xTimers, 200);
						ulNotifiedValue = (EM_OFF << 0) | (LIGHT_OFF << 1) | (BUZZER_OFF << 2) | (USER_CANCEL << 3);
						printf("Microwave Cancelled...\n");
						break;
					case TIMER_EXPIRED:
						ulNotifiedValue = (EM_OFF << 0) | (LIGHT_ON << 1) | (BUZZER_ON << 2) | (TIMER_EXPIRED << 3);
						printf("Times Up! Food is ready...\n");
						break;
					case C_FAILURE:
						ulCond = C_FAILURE;
						xTimerStop(xTimers, 200);
						ulNotifiedValue = (EM_OFF << 0) | (LIGHT_ON << 1) | (BUZZER_ON << 2) | (C_FAILURE << 3);
						printf("Condition Fail...\n");
						break;
					case POWER_OFF:
						ulCond = POWER_OFF;
						xTimerStop(xTimers, 200);
						ulNotifiedValue = (EM_OFF << 0) | (LIGHT_OFF << 1) | (BUZZER_OFF << 2) | (POWER_OFF << 3);
						break;
				}
				break;

			case C_FAILURE:
				switch (ulReceivedValue)
				{
					case POWER_OFF:
						ulCond = POWER_OFF;
						xTimerStop(xTimers, 200);
						ulNotifiedValue = (EM_OFF << 0) | (LIGHT_ON << 1) | (BUZZER_ON << 2) | (POWER_OFF << 3);
						break;

					case C_NORMAL:
						ulCond = C_NORMAL;
						ulNotifiedValue = (EM_OFF << 0) | (LIGHT_OFF << 1) | (BUZZER_OFF << 2) | (C_NORMAL << 3);
						printf("Condition Normal...\n");
						break;
				}
				break;
		}

		xTaskNotify(hPrintOutput, ulNotifiedValue, eSetValueWithOverwrite);
	}
}

void PrintOutput(void *pvParameters)
{
	(void)pvParameters;
	uint32_t ulReceivedValue; 
	uint32_t ulRemainingTick = 0;
	uint32_t ulEM = 0, ulLight = 0, ulBuzzer = 0, ulState = DEFAULT_STATE;
	uint32_t beepCnt = 2,  timesUpBeepCnt = 0; //Buzzer beeping count when times up or microwave failed.
	BaseType_t isNotifyReceived;
	
	for (;;)
	{
		uint32_t xNextWakeTime = xTaskGetTickCount();

		isNotifyReceived = xTaskNotifyWait(0x00, ULONG_MAX, &ulReceivedValue, 0);

		//set only when task is notified and received value
		if (pdTRUE == isNotifyReceived)
		{
			ulEM	= (ulReceivedValue >> 0) & 0x1;
			ulLight	= (ulReceivedValue >> 1) & 0x1;
			ulBuzzer= (ulReceivedValue >> 2) & 0x1;
			ulState	= (ulReceivedValue >> 3);
		}

		switch (ulState)
		{
			case TIMER_5s:
			case TIMER_10s:
			case TIMER_15s:
			case USER_RESUME:
				ulRemainingTick = xTimerGetExpiryTime(xTimers) - xTaskGetTickCount();
				printf("EM = %d, LIGHT = %d, BUZZER = %d   %ds left...\n", ulEM, ulLight, ulBuzzer, (ulRemainingTick/1000));
				break;
			case USER_PAUSE:
				printf("EM = %d, LIGHT = %d, BUZZER = %d   %ds left...\n", ulEM, ulLight, ulBuzzer, (ulRemainingTick/1000));
				break;
			case TIMER_EXPIRED:
				printf("EM = %d, LIGHT = %d, BUZZER = %d\n", ulEM, ulLight, ulBuzzer);

				timesUpBeepCnt++;
				if (timesUpBeepCnt == 3)
				{
					ulState = DEFAULT_STATE;//after beep 3s, do nothing (done)
					timesUpBeepCnt = 0; //reset
				}
				break;
			case USER_CANCEL:
			case POWER_OFF:
				printf("EM = %d, LIGHT = %d, BUZZER = %d\n", ulEM, ulLight, ulBuzzer);
				ulState = DEFAULT_STATE; //do nothing (done)
				break; 
			case C_NORMAL:
				//beepCnt = 2;
				printf("EM = %d, LIGHT = %d, BUZZER = %d   normal...\n", ulEM, ulLight, ulBuzzer);
				break;
			case C_FAILURE:
				//to alternate beep on and off
				if (beepCnt % 2 == 0)
				{
					printf("EM = %d, LIGHT = %d, BUZZER = %d   failure...\n", ulEM, ulLight, ulBuzzer);
				}
				else
				{
					printf("EM = 0, LIGHT = 0, BUZZER = 0   failure...\n");
				}
				beepCnt++;
				break;
			case DEFAULT_STATE:
				break;
		}

		//Loop every 1s and only print when task is notified 
		vTaskDelayUntil(&xNextWakeTime, pdMS_TO_TICKS(1000UL));

	}
}

void PrintUserOptions()
{
	printf("Press 1: 5s  Express cook\n");
	printf("Press 2: 10s Express cook\n");
	printf("Press 3: 15s Express cook\n");
	printf("Press p: To Pause\n");
	printf("Press r: To Resume\n");
	printf("Press c: To Cancel\n");
	printf("Press 8: Power ON\n");
	printf("Press 9: Power OFF\n");
	printf("Press n: Condition Normal\n");
	printf("Press f: Condition Failure\n\n");
}