// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2020-2021 NXP
 */

#include "tc_gpio.h"

#include "debug_console.h"

#include "FreeRTOS.h"
#include "task.h"

#if GEUL_DEMO_GPIO_TEST

#define MAX_GPIO_PINS_TO_TEST 9
const uint8_t ucGpioPinsSets[9] = {8,9,10,11,24,25,26,27,28};

void printGpioPinsValues(uint32_t vGpioDataRegValue)
{
	/* To remove unused variable warnings */
	(void)vGpioDataRegValue;

        for (uint8_t i = 0; i < MAX_GPIO_PINS_TO_TEST; i++)
        {

                log_info(" Pin %d : Value = %d \n\r", ucGpioPinsSets[i], ((vGpioDataRegValue >> (31 - ucGpioPinsSets[i])) & 0x1));
        }
}

void vResetGpioPins()
{
        uint8_t i;
        uint32_t gpioDataRegValue;

        for (i = 0; i < MAX_GPIO_PINS_TO_TEST; i++)
        {
                exGpioSetData( GPIO_3, ucGpioPinsSets[i], 0 );
        }

        exGpioGetDataRegister(GPIO_3, &gpioDataRegValue);
        log_info("Output of gpio pins after Reset ALL\n\r");
        printGpioPinsValues(gpioDataRegValue);
}

void vSetGpioPins()
{
        uint8_t i;
        uint32_t gpioDataRegValue;

        for (i = 0; i < MAX_GPIO_PINS_TO_TEST; i++)
        {
                exGpioSetData( GPIO_3, ucGpioPinsSets[i], 1 );
        }

        exGpioGetDataRegister(GPIO_3, &gpioDataRegValue);
        log_info("Output of gpio pins after setting ALL pins to 1\n\r");
        printGpioPinsValues(gpioDataRegValue);


}


void vResetAlternativeGpioPins()
{
        uint32_t gpioDataRegValue;
        
        vSetGpioPins();
        
        exGpioSetData( GPIO_3, ucGpioPinsSets[0], 0 );
        exGpioGetDataRegister(GPIO_3, &gpioDataRegValue);
        log_info("\nOutput of gpio pins after Resetting only GPIO pin (%d) to 0 and seting others to 1: \n\r", ucGpioPinsSets[0]);
        printGpioPinsValues(gpioDataRegValue);

        for (uint8_t i = 1; i < MAX_GPIO_PINS_TO_TEST; i++)
        {
                vTaskDelay(1);
                exGpioSetData( GPIO_3, ucGpioPinsSets[i-1], 1 );
                exGpioSetData( GPIO_3, ucGpioPinsSets[i], 0 );
                exGpioGetDataRegister(GPIO_3, &gpioDataRegValue);
                log_info("\nOutput of gpio pins after Resetting only GPIO pin (%d) to 0 and seting others to 1: \n\r", ucGpioPinsSets[i]);
                printGpioPinsValues(gpioDataRegValue);
        }

}
void vSetAlternativeGpioPins()
{
        uint32_t gpioDataRegValue;
        
	vResetGpioPins();
        
        exGpioSetData( GPIO_3, ucGpioPinsSets[0], 1 );
        exGpioGetDataRegister(GPIO_3, &gpioDataRegValue);
        log_info("\nOutput of gpio pins after Setting only GPIO pin (%d) to 1 and reseting others: \n\r", ucGpioPinsSets[0]);
        printGpioPinsValues(gpioDataRegValue);

        for (uint8_t i = 1; i < MAX_GPIO_PINS_TO_TEST; i++)
        {
                vTaskDelay(1);
                exGpioSetData( GPIO_3, ucGpioPinsSets[i-1], 0 );
                exGpioSetData( GPIO_3, ucGpioPinsSets[i], 1 );
                exGpioGetDataRegister(GPIO_3, &gpioDataRegValue);
                log_info("\nOutput of gpio pins after Setting only GPIO pin (%d) to 1 and reseting others: \n\r", ucGpioPinsSets[i]);
                printGpioPinsValues(gpioDataRegValue);
        }
}



void vInitGpioPins()
{
	uint8_t i;

	for (i = 0; i < MAX_GPIO_PINS_TO_TEST; i++)
	{
		if(GPIO_SUCCESS != exGpioInit( GPIO_3 , ucGpioPinsSets[i], GPIO_OUTPUT ))
		{
			log_info( " GPIO init failed\n\r" );
		}
	}
}

/*void vGpioGenericTest(void)
{

        GpioModule_t exGpioModule;
        uint8_t ucPin;

        log_info("%s : Started\n\r", __func__);
        for ( exGpioModule = GPIO_0 ; exGpioModule < GPIO_MAX; exGpioModule++ )
        {
                for ( ucPin = 0; ucPin < 32; ucPin++ )
                {
                //      log_info("%s : GpioModule : %d ucPin : %d\n\r", __func__, exGpioModule, ucPin);
                        exGpioInit( exGpioModule , ucPin, GPIO_INPUT );
                        exGpioSetInputData( exGpioModule, ucPin, 1 );
                        exGpioGetData( exGpioModule, ucPin );
                        exGpioSetInputData( exGpioModule, ucPin, 0 );
                        exGpioGetData( exGpioModule, ucPin );
                }
        }
}*/
void vGpioTest( void )
{
        u32 uiCurrentCore = ulMpicCurrentCore();

        //vGpioGenericTest();

        vInitGpioPins();
        
        vSetAlternativeGpioPins();
        vResetAlternativeGpioPins();
	SET_TEST_STATUS( uiCurrentCore, GEUL_DEMO_GPIO_TEST_STATUS );
}
#endif

int32_t gpioLedTestCase( GpioModule_t ucGpioModule, uint8_t ucPin )
{
	uint32_t gpioDataRegValue=0;
	GpioStatusCode_t ret;
	
	ret = exGpioInit( ucGpioModule - 1, ucPin, GPIO_OUTPUT );
	if (GPIO_SUCCESS != ret)
	{
		log_info("\n GPIO init failed for gpio module = %d, pin =%d", ucGpioModule, ucPin);
		return -1;
	}
	exGpioGetDataRegister(ucGpioModule - 1, &gpioDataRegValue);
	log_info("\nOutput of gpio %d data  = %x \n\r", ucGpioModule, gpioDataRegValue);
	while (1)
	{
		exGpioSetData( ucGpioModule - 1, ucPin, 0 );
		exGpioGetDataRegister(ucGpioModule - 1, &gpioDataRegValue);
		log_info("\nOutput of gpio %d data  = %x \n\r", ucGpioModule, gpioDataRegValue);
		vTaskDelay(1000);
		exGpioSetData( ucGpioModule - 1, ucPin, 1 );
		exGpioGetDataRegister(ucGpioModule - 1, &gpioDataRegValue);
		log_info("\nOutput of gpio %d data  = %x \n\r", ucGpioModule, gpioDataRegValue);
		vTaskDelay(1000);
	}

	return 0;
	
}
