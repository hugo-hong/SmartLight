/**********************************************************************************
 * Copyright(C) 2018, blueberry LLC
 * All Rights Reserved.
 *
 * Confidential Information of 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of ;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of blueberry LLC.
 * 
 * **************************** SMART LIGHT LIBRARY ****************************
 * This module contains the code for the Smart Light Library. This Library
 * abstracts the specific lighting part and provides APIs that can be 
 * called to perform various operations on the Light bulbs. This is designed to be 
 * a generic interface that can be used by any protocol (Zigbee, Zwave, Bluetooth)
 * 
***********************************************************************************/
/*  
 * @file:led_type.h
 * @brief:define basic type for smart light library
 * @author:hugo.hong
 * @data:2018-08-01
 * @history:
*/ 
#ifndef _SLL_TYPE_H_
#define _SLL_TYPE_H_

#include <stdint.h>
#include "wiced.h"
#include "wiced_bt_trace.h"
#include "wiced_hal_puart.h"

#define LOG_TAG "LSL_LIGHT"

/* Boolean values */
#ifndef bool_t
#define bool_t unsigned char
#endif
#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NULL
#define NULL ((void*)0)
#endif
#ifndef OFF
#define OFF	  0
#endif
#ifndef ON
#define ON	  1
#endif

#ifdef WICED_BT_TRACE_ENABLE
#define LSL_LOG_TRACE(tag, format, ...) WICED_BT_TRACE("[%s]%s:" format"\n", tag, __FUNCTION__, ##__VA_ARGS__)
#else
#define LSL_LOG_TRACE(tag, format, ...)
#endif

#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef DIM
#define DIM(a) (sizeof(a) / sizeof(a[0]))
#endif

#ifndef OFFSETOF
#define OFFSETOF(type, member) ((size_t)(&((type*)0)->member))
#endif

#ifndef MEMBER_OF
#define MEMBER_OF(ptr, type, member) ((typeof( ((type *)0)->member ) *)( (char *)ptr + OFFSETOF(type, member) ))
#endif

#ifndef CONTAINER_OF
#define CONTAINER_OF(ptr, type, member) ({\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);\
        (type *)( (char *)__mptr - OFFSETOF(type, member) );\
      })
#endif



typedef enum _sll_status {
  SLL_SUCCESS = 0,
  SLL_FAIL,
  SLL_UNKNOWN
}sll_status_t;





#endif //_SLL_TYPE_H_
