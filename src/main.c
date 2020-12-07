/*
 * This file is part of the 碌OS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2014 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include "diag/Trace.h"

#include "stm32f4xx_hal.h"

#include "stm32f4.frsvd/dma_uart.h"
#include "stm32f4.frsvd/dwt_cycle.h"
#include "frsvd/inc/tsk.h"
#include "sys/shell.h"
#include "app/commands.h"
#include "app/controller.h"
#include "app/gamepad.h"
#include "app/pwm.h"

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"


int main(int argc, char* argv[])
{
    cycInit();
    dmaUartInit(0, 115200, 128);

    printf("System clock: %lu Hz\r\n", SystemCoreClock);

    tskInit(1000, HAL_GetTick);

    shellInit();
    shellOn();

    regGamepadCmds();
    regPwmCmds();

    ctrlMcbInit();
    
    gpTskInit();
    pwmTskInit();
    ctrlTskInit();

    for(;;)
    {
        shellChk();
        tskExec();
    }
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------------
