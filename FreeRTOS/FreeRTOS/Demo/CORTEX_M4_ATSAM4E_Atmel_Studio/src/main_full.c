/*
    FreeRTOS V7.6.0 - Copyright (C) 2013 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that has become a de facto standard.             *
     *                                                                       *
     *    Help yourself get started quickly and support the FreeRTOS         *
     *    project by purchasing a FreeRTOS tutorial book, reference          *
     *    manual, or both from: http://www.FreeRTOS.org/Documentation        *
     *                                                                       *
     *    Thank you!                                                         *
     *                                                                       *
    ***************************************************************************

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

    >>! NOTE: The modification to the GPL is included to allow you to distribute
    >>! a combined work that includes FreeRTOS without being obliged to provide
    >>! the source code for proprietary components outside of the FreeRTOS
    >>! kernel.

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available from the following
    link: http://www.freertos.org/a00114.html

    1 tab == 4 spaces!

    ***************************************************************************
     *                                                                       *
     *    Having a problem?  Start by reading the FAQ "My application does   *
     *    not run, what could be wrong?"                                     *
     *                                                                       *
     *    http://www.FreeRTOS.org/FAQHelp.html                               *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org - Documentation, books, training, latest versions,
    license and Real Time Engineers Ltd. contact details.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.OpenRTOS.com - Real Time Engineers ltd license FreeRTOS to High
    Integrity Systems to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/******************************************************************************
 * NOTE 1:  This project provides two demo applications.  A simple blinky style
 * project, and a more comprehensive test and demo application that makes use of
 * the FreeRTOS+CLI, FreeRTOS+UDP and FreeRTOS+FAT SL components.  The
 * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting in main.c is used to select
 * between the two.  See the notes on using mainCREATE_SIMPLE_BLINKY_DEMO_ONLY
 * in main.c.  This file implements the comprehensive test and demo version,
 * which is fully documented on the following URL:
 * http://www.FreeRTOS.org/Atmel_SAM4E_RTOS_Demo.html
 *
 * NOTE 2:  This file only contains the source code that is specific to the
 * full demo.  Generic functions, such FreeRTOS hook functions, and functions
 * required to configure the hardware, are defined in main.c.
 ******************************************************************************
 *
 * Full user instructions are provided on the following URL:
 * http://www.FreeRTOS.org/Atmel_SAM4E_RTOS_Demo.html
 *
 * main_full():
 * 	+ Uses FreeRTOS+FAT SL to create a set of example files on a RAM disk.
 *  + Displays some bitmaps on the LCD.
 *  + Registers sample generic, file system related and UDP related commands
 *	  with FreeRTOS+CLI.
 *	+ Creates all the standard demo application tasks and software timers.
 *	+ Starts the scheduler.
 *
 * A UDP command server and optionally two UDP echo client tasks are created
 * from the network event hook after an IP address has been obtained.  The IP
 * address is displayed on the LCD.
 *
 * A "check software timer" is created to provide visual feedback of the system
 * status.  The timer's period is initially set to three seconds.  The callback
 * function associated with the timer checks all the standard demo tasks are not
 * only still executed, but are executing without reporting any errors.  If the
 * timer discovers a task has either stalled, or reported an error, then it
 * changes its own period from the initial three seconds, to just 200ms.  The
 * check software timer also toggles the LED marked D4 - so if the LED toggles
 * every three seconds then no potential errors have been found, and if the LED
 * toggles every 200ms then a potential error has been found in at least one
 * task.
 *
 * Information on accessing the CLI and file system, and using the UDP echo
 * tasks is provided on http://www.FreeRTOS.org/Atmel_SAM4E_RTOS_Demo.html
 *
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

/* FreeRTOS+UDP includes. */
#include "FreeRTOS_UDP_IP.h"
#include "FreeRTOS_Sockets.h"

/* UDP demo includes. */
#include "UDPCommandInterpreter.h"
#include "TwoEchoClients.h"

/* Standard demo includes. */
#include "partest.h"
#include "blocktim.h"
#include "flash_timer.h"
#include "semtest.h"
#include "GenQTest.h"
#include "QPeek.h"
#include "IntQueue.h"
#include "countsem.h"
#include "dynamic.h"
#include "QueueOverwrite.h"
#include "QueueSet.h"
#include "recmutex.h"

/* The period after which the check timer will expire, in ms, provided no errors
have been reported by any of the standard demo tasks.  ms are converted to the
equivalent in ticks using the portTICK_RATE_MS constant. */
#define mainCHECK_TIMER_PERIOD_MS			( 3000UL / portTICK_RATE_MS )

/* The period at which the check timer will expire, in ms, if an error has been
reported in one of the standard demo tasks.  ms are converted to the equivalent
in ticks using the portTICK_RATE_MS constant. */
#define mainERROR_CHECK_TIMER_PERIOD_MS 	( 200UL / portTICK_RATE_MS )

/* The priorities of the various demo application tasks. */
#define mainSEM_TEST_PRIORITY				( tskIDLE_PRIORITY + 1 )
#define mainBLOCK_Q_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define mainCOM_TEST_PRIORITY				( tskIDLE_PRIORITY + 2 )
#define mainINTEGER_TASK_PRIORITY           ( tskIDLE_PRIORITY )
#define mainGEN_QUEUE_TASK_PRIORITY			( tskIDLE_PRIORITY )
#define mainQUEUE_OVERWRITE_TASK_PRIORITY	( tskIDLE_PRIORITY )

/* The LED controlled by the 'check' software timer. */
#define mainCHECK_LED						( 2 )

/* The number of LEDs that should be controlled by the flash software timer
standard demo.  In this case it is only 1 as the starter kit has three LEDs, one
of which is controlled by the check timer and one of which is controlled by the
ISR triggered task. */
#define mainNUM_FLASH_TIMER_LEDS			( 1 )

/* Misc. */
#define mainDONT_BLOCK						( 0 )

/* Note:  If the application is started without the network cable plugged in
then ipconfigUDP_TASK_PRIORITY should be set to 0 in FreeRTOSIPConfig.h to
ensure the IP task is created at the idle priority.  This is because the Atmel
ASF GMAC driver polls the GMAC looking for a connection, and doing so will
prevent any lower priority tasks from executing.  In this demo the IP task is
started at the idle priority, then set to configMAX_PRIORITIES - 2 in the
network event hook only after a connection has been established (when the event
passed into the network event hook is eNetworkUp).
http://www.FreeRTOS.org/udp */
#define mainCONNECTED_IP_TASK_PRIORITY		( configMAX_PRIORITIES - 1 )
#define mainDISCONNECTED_IP_TASK_PRIORITY	( tskIDLE_PRIORITY )

/* UDP command server task parameters. */
#define mainUDP_CLI_TASK_PRIORITY			( tskIDLE_PRIORITY )
#define mainUDP_CLI_PORT_NUMBER				( 5001UL )
#define mainUDP_CLI_TASK_STACK_SIZE			( configMINIMAL_STACK_SIZE * 2U )

/* Set to 1 to include the UDP echo client tasks in the build.  The echo clients
require the IP address of the echo server to be defined using the
configECHO_SERVER_ADDR0 to configECHO_SERVER_ADDR3 constants in
FreeRTOSConfig.h. */
#define mainINCLUDE_ECHO_CLIENT_TASKS		1

/*-----------------------------------------------------------*/

/*
 * The check timer callback function, as described at the top of this file.
 */
static void prvCheckTimerCallback( xTimerHandle xTimer );

/*
 * Creates a set of sample files on a RAM disk.  http://www.FreeRTOS.org/fat_sl
 */
extern void vCreateAndVerifySampleFiles( void );

/*
 * Register sample generic commands that can be used with FreeRTOS+CLI.  Type
 * 'help' in the command line to see a list of registered commands.
 * http://www.FreeRTOS.org/cli
 */
extern void vRegisterSampleCLICommands( void );

/*
 * Register sample file system commands that can be used with FreeRTOS+CLI.
 */
extern void vRegisterFileSystemCLICommands( void );

/*
 * Register sample UDP related commands that can be used with FreeRTOS+CLI.
 */
extern void vRegisterUDPCLICommands( void );

/*
 * Initialise the LCD and output a bitmap.
 */
extern void vInitialiseLCD( void );

/*-----------------------------------------------------------*/

/* The default IP and MAC address used by the demo.  The address configuration
defined here will be used if ipconfigUSE_DHCP is 0, or if ipconfigUSE_DHCP is
1 but a DHCP server could not be contacted.  See the online documentation for
more information. */
static const uint8_t ucIPAddress[ 4 ] = { configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3 };
static const uint8_t ucNetMask[ 4 ] = { configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3 };
static const uint8_t ucGatewayAddress[ 4 ] = { configGATEWAY_ADDR0, configGATEWAY_ADDR1, configGATEWAY_ADDR2, configGATEWAY_ADDR3 };
static const uint8_t ucDNSServerAddress[ 4 ] = { configDNS_SERVER_ADDR0, configDNS_SERVER_ADDR1, configDNS_SERVER_ADDR2, configDNS_SERVER_ADDR3 };

/* The MAC address used by the demo.  In production units the MAC address would
probably be read from flash memory or an EEPROM.  Here it is just hard coded.
Note each node on a network must have a unique MAC address. */
const uint8_t ucMACAddress[ 6 ] = { configMAC_ADDR0, configMAC_ADDR1, configMAC_ADDR2, configMAC_ADDR3, configMAC_ADDR4, configMAC_ADDR5 };

/*-----------------------------------------------------------*/

int main_full( void )
{
xTimerHandle xTimer = NULL;

	/* Usage instructions on http://www.FreeRTOS.org/Atmel_SAM4E_RTOS_Demo.html */

	/* Initialise the LCD and output a bitmap.  The IP address will also be
	displayed on the LCD when it has been obtained. */
	vInitialiseLCD();

	/* If the file system is only going to be accessed from one task then
	F_FS_THREAD_AWARE can be set to 0 and the set of example files are created
	before the RTOS scheduler is started.  If the file system is going to be
	access from more than one task then F_FS_THREAD_AWARE must be set to 1 and
	the	set of sample files are created from the idle task hook function
	vApplicationIdleHook(). */
	#if( F_FS_THREAD_AWARE == 0 )
	{
		/* Initialise the drive and file system, then create a few example
		files.  The files can be viewed and accessed via the CLI.  View the
		documentation page for this demo (link at the top of this file) for more
		information. */
		vCreateAndVerifySampleFiles();
	}
	#endif

	/* Register example generic, file system related and UDP related CLI
	commands respectively.  Type 'help' into the command console to view a list
	of registered commands. */
	vRegisterSampleCLICommands();
	vRegisterFileSystemCLICommands();
	vRegisterUDPCLICommands();

	/* Initialise the network interface.  Tasks that use the network are
	created in the network event hook when the network is connected and ready
	for use.  The address values passed in here are used if ipconfigUSE_DHCP is
	set to 0, or if ipconfigUSE_DHCP is set to 1 but a DHCP server cannot be
	contacted.  The IP address actually used is displayed on the LCD (after DHCP
	has completed if DHCP is used). */
	FreeRTOS_IPInit( ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress );

	/* Create all the other standard demo tasks. */
	vStartLEDFlashTimers( mainNUM_FLASH_TIMER_LEDS );
	vCreateBlockTimeTasks();
	vStartSemaphoreTasks( mainSEM_TEST_PRIORITY );
	vStartGenericQueueTasks( mainGEN_QUEUE_TASK_PRIORITY );
	vStartQueuePeekTasks();
	vStartCountingSemaphoreTasks();
	vStartDynamicPriorityTasks();
	vStartQueueOverwriteTask( mainQUEUE_OVERWRITE_TASK_PRIORITY );
	vStartQueueSetTasks();
	vStartRecursiveMutexTasks();

	/* Create the software timer that performs the 'check' functionality, as
	described at the top of this file. */
	xTimer = xTimerCreate( 	( const signed char * ) "CheckTimer",/* A text name, purely to help debugging. */
							( mainCHECK_TIMER_PERIOD_MS ),		/* The timer period, in this case 3000ms (3s). */
							pdTRUE,								/* This is an auto-reload timer, so xAutoReload is set to pdTRUE. */
							( void * ) 0,						/* The ID is not used, so can be set to anything. */
							prvCheckTimerCallback );			/* The callback function that inspects the status of all the other tasks. */

	if( xTimer != NULL )
	{
		xTimerStart( xTimer, mainDONT_BLOCK );
	}

	/* Start the scheduler itself. */
	vTaskStartScheduler();

	/* If all is well, the scheduler will now be running, and the following line
	will never be reached.  If the following line does execute, then there was
	insufficient FreeRTOS heap memory available for the idle and/or timer tasks
	to be created.  See the memory management section on the FreeRTOS web site
	for more details. */
	for( ;; );
}
/*-----------------------------------------------------------*/

static void prvCheckTimerCallback( xTimerHandle xTimer )
{
static long lChangedTimerPeriodAlready = pdFALSE;
unsigned long ulErrorOccurred = pdFALSE;

	/* Avoid compiler warnings. */
	( void ) xTimer;

	/* Have any of the standard demo tasks detected an error in their
	operation? */
	if( xAreGenericQueueTasksStillRunning() != pdTRUE )
	{
		ulErrorOccurred |= ( 0x01UL << 3UL );
	}
	else if( xAreQueuePeekTasksStillRunning() != pdTRUE )
	{
		ulErrorOccurred |= ( 0x01UL << 4UL );
	}
	else if( xAreBlockTimeTestTasksStillRunning() != pdTRUE )
	{
		ulErrorOccurred |= ( 0x01UL << 5UL );
	}
	else if( xAreSemaphoreTasksStillRunning() != pdTRUE )
	{
		ulErrorOccurred |= ( 0x01UL << 6UL );
	}
	else if( xAreCountingSemaphoreTasksStillRunning() != pdTRUE )
	{
		ulErrorOccurred |= ( 0x01UL << 8UL );
	}
	else if( xAreDynamicPriorityTasksStillRunning() != pdTRUE )
	{
		ulErrorOccurred |= ( 0x01UL << 9UL );
	}
	else if( xIsQueueOverwriteTaskStillRunning() != pdTRUE )
	{
		ulErrorOccurred |= ( 0x01UL << 10UL );
	}
	else if( xAreQueueSetTasksStillRunning() != pdTRUE )
	{
		ulErrorOccurred |= ( 0x01UL << 11UL );
	}
	else if( xAreRecursiveMutexTasksStillRunning() != pdTRUE )
	{
		ulErrorOccurred |= ( 0x01UL << 12UL );
	}

	if( ulErrorOccurred != pdFALSE )
	{
		/* An error occurred.  Increase the frequency at which the check timer
		toggles its LED to give visual feedback of the potential error
		condition. */
		if( lChangedTimerPeriodAlready == pdFALSE )
		{
			lChangedTimerPeriodAlready = pdTRUE;

			/* This call to xTimerChangePeriod() uses a zero block time.
			Functions called from inside of a timer callback function must
			*never* attempt	to block as to do so could impact other software
			timers. */
			xTimerChangePeriod( xTimer, ( mainERROR_CHECK_TIMER_PERIOD_MS ), mainDONT_BLOCK );
		}
	}

	/* Toggle the LED to give visual feedback of the system status.  The rate at
	which the LED toggles will increase to mainERROR_CHECK_TIMER_PERIOD_MS if a
	suspected error has been found in any of the standard demo tasks. */
	vParTestToggleLED( mainCHECK_LED );
}
/*-----------------------------------------------------------*/

/* Called by FreeRTOS+UDP when the network connects. */
void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent )
{
static long lTasksAlreadyCreated = pdFALSE;
const unsigned long ulXCoord = 3, ulYCoord = 3, ulIPAddressOffset = 45;
unsigned long ulIPAddress;
char cIPAddress[ 20 ];

	/* Note:  If the application is started without the network cable plugged in
	then ipconfigUDP_TASK_PRIORITY should be set to 0 in FreeRTOSIPConfig.h to
	ensure the IP task is created at the idle priority.  This is because the Atmel
	ASF GMAC driver polls the GMAC looking for a connection, and doing so will
	prevent any lower priority tasks from executing.  In this demo the IP task is
	started at the idle priority, then set to configMAX_PRIORITIES - 2 in the
	network event hook only after a connection has been established (when the event
	passed into the network event hook is eNetworkUp). */
	if( eNetworkEvent == eNetworkUp )
	{
		/* Ensure tasks are only created once. */
		if( lTasksAlreadyCreated == pdFALSE )
		{
			/* Create the task that handles the CLI on a UDP port.  The port
			number is set using the configUDP_CLI_PORT_NUMBER setting in
			FreeRTOSConfig.h. */
			vStartUDPCommandInterpreterTask( mainUDP_CLI_TASK_STACK_SIZE, mainUDP_CLI_PORT_NUMBER, mainUDP_CLI_TASK_PRIORITY );

			#if( mainINCLUDE_ECHO_CLIENT_TASKS == 1 )
			{
				/* Create the UDP echo tasks.  The UDP echo tasks require the IP
				address of the echo server to be defined using the
				configECHO_SERVER_ADDR0 to configECHO_SERVER_ADDR3 constants in
				FreeRTOSConfig.h. */
				vStartEchoClientTasks( configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY );
			}
			#endif
		}

		/* Obtain the IP address, convert it to a string, then display it on the
		LCD. */
		FreeRTOS_GetAddressConfiguration( &ulIPAddress, NULL, NULL, NULL );
		FreeRTOS_inet_ntoa( ulIPAddress, cIPAddress );
		ili93xx_draw_string( ulXCoord, ulYCoord, ( uint8_t * ) "IP: " );
		ili93xx_draw_string( ulXCoord + ulIPAddressOffset, ulYCoord, ( uint8_t * ) cIPAddress );

		/* Set the priority of the IP task up to the desired priority now it has
		connected. */
		vTaskPrioritySet( NULL, mainCONNECTED_IP_TASK_PRIORITY );
	}

	/* NOTE:  At the time of writing the Ethernet driver does not report the
	cable being unplugged - so the following if() condition will never be met.
	It is included for possible future updates to the driver. */
	if( eNetworkEvent == eNetworkDown )
	{
		/* Ensure the Atmel GMAC drivers don't hog all the CPU time as they look
		for a new connection by lowering the priority of the IP task to that of
		the Idle task. */
		vTaskPrioritySet( NULL, tskIDLE_PRIORITY );
		
		/* Disconnected - so no IP address. */
		ili93xx_draw_string( ulXCoord, ulYCoord, ( uint8_t * ) "IP:                  " );
	}
}
/*-----------------------------------------------------------*/

void vFullDemoIdleHook( void )
{
	/* If the file system is only going to be accessed from one task then
	F_FS_THREAD_AWARE can be set to 0 and the set of example files is created
	before the RTOS scheduler is started.  If the file system is going to be
	access from more than one task then F_FS_THREAD_AWARE must be set to 1 and
	the	set of sample files are created from the idle task hook function. */
	#if( F_FS_THREAD_AWARE == 1 )
	{
		static portBASE_TYPE xCreatedSampleFiles = pdFALSE;

		/* Initialise the drive and file system, then create a few example
		files.  The output from this function just goes to the stdout window,
		allowing the output to be viewed when the UDP command console is not
		connected. */
		if( xCreatedSampleFiles == pdFALSE )
		{
			vCreateAndVerifySampleFiles();
			xCreatedSampleFiles = pdTRUE;
		}
	}
	#endif
}
/*-----------------------------------------------------------*/

void vFullDemoTickHook( void )
{
	/* Call the periodic queue overwrite from ISR test function. */
	vQueueOverwritePeriodicISRDemo();

	/* Call the periodic queue set ISR test function. */
	vQueueSetAccessQueueSetFromISR();
}
/*-----------------------------------------------------------*/

/* Called automatically when a reply to an outgoing ping is received. */
void vApplicationPingReplyHook( ePingReplyStatus_t eStatus, uint16_t usIdentifier )
{
	/* This demo has nowhere to output any information so does nothing, but the
	IP address resolved for the pined URL is displayed in the CLI. */
	( void ) usIdentifier;
	( void ) eStatus;
}
/*-----------------------------------------------------------*/

