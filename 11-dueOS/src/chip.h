/*
  Copyright (c) 2011 Arduino.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _LIB_SAM_
#define _LIB_SAM_

/*
 * Core and peripherals registers definitions
 */
#include <sam/sam.h>

/* Define attribute */
#if defined (  __GNUC__  ) /* GCC CS3 */
    #define WEAK __attribute__ ((weak))
#elif defined ( __ICCARM__ ) /* IAR Ewarm 5.41+ */
    #define WEAK __weak
#endif

/* Define NO_INIT attribute */
#if defined (  __GNUC__  )
    #define NO_INIT
#elif defined ( __ICCARM__ )
    #define NO_INIT __no_init
#endif

/*
 * Peripherals
 */
#include "sam/include/adc.h"
#if (SAM3XA_SERIES) || (SAM3N_SERIES) || (SAM3S_SERIES)
#include "sam/include/dacc.h"
#endif // (SAM3XA_SERIES) || (SAM3N_SERIES) || (SAM3S_SERIES)

#include "sam/include/interrupt_sam_nvic.h"
#include "sam/include/efc.h"
#include "sam/include/gpbr.h"
#include "sam/include/pio.h"
#include "sam/include/pmc.h"
#include "sam/include/pwmc.h"
#include "sam/include/rstc.h"
#include "sam/include/rtc.h"
#include "sam/include/rtt.h"
#include "sam/include/spi.h"
#include "sam/include/ssc.h"
#include "sam/include/tc.h"
#include "sam/include/twi.h"
#include "sam/include/usart.h"
#include "sam/include/wdt.h"

#include "sam/include/timetick.h"
#include "sam/include/USB_device.h"
#include "sam/include/USB_host.h"

#if (SAM3XA_SERIES)
#include "sam/include/can.h"
#include "sam/include/emac.h"
#include "sam/include/trng.h"
#include "sam/include/uotghs_device.h"
#include "sam/include/uotghs_host.h"
#endif /* (SAM3XA_SERIES) */

#endif /* _LIB_SAM_ */
