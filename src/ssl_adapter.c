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
 * @file:light_adapter.c
 * @brief:implement interface for ledvance light bulbs
 * @author:hugo.hong
 * @data:2018-07-16
 * @history:
*/ 
#include "sll_type.h"
#include "sll_hal.h"
#include "sll_util.h"
#include "sll_characteristics.h"
#include "sll_control.h"
#include "sll_driver.h"

typedef struct {
    lsl_light_status_t bulbState;
    task_action task_cb;
}light_status_t;

static light_status_t kBulbStatus = {0};


static void task_sequence_ind_cb(uint8_t action) {
    uint16_t cur_tt, seq_tt = 50;
    switch (action) {
    case ACTION_DOING:
        lightCharas_getProperty(TT, &cur_tt);        
        //do flush
        light_switchOn(seq_tt, NULL);
        //restore tt
        lightCharas_setProperty(TT, &cur_tt);
        break;
    case ACTION_DONE:       
    case ACTION_NONE:
    default:
        //do nothing
        break; 
    }
}

//this function invoked after a task done
static void task_updateStatus_cb(uint8_t action) {
    switch (action) {
    case ACTION_DOING:
        kBulbStatus.bulbState.time_remaining = (kBulbStatus.bulbState.time_remaining > LC_DEFAULT_DIM_RISE_GRANULARITY ? 
                                                kBulbStatus.bulbState.time_remaining - LC_DEFAULT_DIM_RISE_GRANULARITY :
                                                LC_DEFAULT_DIM_RISE_GRANULARITY); 
        break;
    case ACTION_DONE:
        lightCharas_getProperty(ONOFF, &kBulbStatus.bulbState.on_off);
        lightCharas_getProperty(BRIGHTNESS, &kBulbStatus.bulbState.brightness);
        lightCharas_getProperty(HUE, &kBulbStatus.bulbState.hue);
        lightCharas_getProperty(SATURATION, &kBulbStatus.bulbState.sat);
        lightCharas_getProperty(CCT, &kBulbStatus.bulbState.cct);
        kBulbStatus.bulbState.time_remaining = 0;         
        break;
    case ACTION_NONE:
    default:
        //do nothing
        break; 
    }

    //notify app
    if (kBulbStatus.task_cb != NULL) kBulbStatus.task_cb(action);    
}

void sll_init(bool_t reset) {
    lightCharas_init(reset);
    lightControl_init();
    lightDriver_init(task_updateStatus_cb);
}

lsl_light_status_t *light_getStatus(void) {
    return &kBulbStatus.bulbState;
}

void sll_turnOn(uint16_t trans_time, task_action task_cb) {
    uint8_t on_off, level, mode;  

    //get current prop
    lightCharas_getProperty(ONOFF, &on_off);
    lightCharas_getProperty(BRIGHTNESS, &level);
    lightCharas_getProperty(COLORMODE, &mode);

    if (on_off == ON) return;

    //update tt
    trans_time = (trans_time * kDimArray[level])/255;

    //update onff 
    on_off = ON;

    //do action
    if (CTL == mode) {
        sll_light_ctl_data_t ctl_data;
        ctl_data.brightness = level;
        ctl_data.trans_time = trans_time;
        lightCharas_getProperty(CCT, &ctl_data.temperature);
        lightDriver_setGlow(&ctl_data, mode, task_cb);
    }
    else if (HSL == mode) {
        lsl_light_hsl_data_t hsl_data;
        hsl_data.brightness = level;
        hsl_data.trans_time = trans_time;
        lightCharas_getProperty(HUE, &hsl_data.hue);
        lightCharas_getProperty(SATURATION, &hsl_data.saturation);
        lightDriver_setGlow(&hsl_data, mode, task_cb);
    }   
}

void sll_turnOff(uint16_t trans_time, task_action task_cb) {
    uint8_t on_off, level, mode;

    //get current prop
    lightCharas_getProperty(ONOFF, &on_off);
    lightCharas_getProperty(BRIGHTNESS, &level);
    lightCharas_getProperty(COLORMODE, &mode);

    if (on_off == OFF || level == 0) return;

    //update tt
    trans_time = (trans_time * kDimArray[level])/255;    

    //update onff 
    on_off = OFF;
    level = 0;
 
    //do action
    if (CTL == mode) {
        lsl_light_ctl_data_t ctl_data;
        ctl_data.brightness = level;
        ctl_data.trans_time = trans_time;
        lightCharas_getProperty(CCT, &ctl_data.temperature);
        light_setCTL(&ctl_data, task_cb);
    }
    else if (HSL == mode) {
        lsl_light_hsl_data_t hsl_data;
        hsl_data.brightness = level;
        hsl_data.trans_time = trans_time;
        lightCharas_getProperty(HUE, &hsl_data.hue);
        lightCharas_getProperty(SATURATION, &hsl_data.saturation);
        light_setHSL(&hsl_data, task_cb);
    }   
}

void light_registerReset(task_action reset_cb) {
    lightControl_triggerResetAction(reset_cb);
}

void sll_startSequence(sll_sequence_mode_t mode) {
    lightControl_triggerSequenceAction(task_sequence_ind_cb, mode);
}

void sll_haltSequnce() {
    //stop sequence task
    REVOKE_TASK_ACTION(kTimers[SEQUENCE]);

    //turn off lights
    light_switchOff(0, NULL);   
}

void sll_setCTL(sll_light_ctl_data_t *p_ctl, task_action task_cb) {
    uint8_t level;
    uint16_t curCCT, minCCT, maxCCT;
    lsl_color_mode_t mode;

    //get current prop
    lightCharas_getProperty(CCT, &curCCT);
    lightCharas_getProperty(CCTMAX, &maxCCT);
    lightCharas_getProperty(CCTMIN, &minCCT);
    lightCharas_getProperty(BRIGHTNESS, &level);
    lightCharas_getProperty(COLORMODE, &mode);

    if (curCCT == p_ctl->temperature && level == p_ctl->brightness && mode == CTL)
        return;

    //update cct
    p_ctl->temperature = MIN(p_ctl->temperature, maxCCT);
    p_ctl->temperature = MAX(p_ctl->temperature, minCCT);

    //update brightness
    p_ctl->brightness = MIN(p_ctl->brightness, 0xFE);

    //update TT
    p_ctl->trans_time = (p_ctl->trans_time == 0xFFFF ? TT_DEFAULT : p_ctl->trans_time); 
    lightCharas_setProperty(TT, &p_ctl->trans_time);

    //do action
    kBulbStatus.task_cb = task_cb;
    lightDriver_setGlow(p_ctl, CTL, task_cb == NULL ? NULL : task_updateStatus_cb);
}

void sll_setHSL(sll_light_hsl_data_t *p_hsl, task_action task_cb) {
    uint8_t hue, sat, level;
    lsl_color_mode_t mode;

    //get current prop
    lightCharas_getProperty(HUE, &hue);
    lightCharas_getProperty(SATURATION, &sat);
    lightCharas_getProperty(BRIGHTNESS, &level);
    lightCharas_getProperty(COLORMODE, &mode);    

    if (hue == p_hsl->hue && sat == p_hsl->saturation && level == p_hsl->brightness && mode == HSL)
        return;

    //update hue & saturation
    p_hsl->hue = MIN(p_hsl->hue, 0xFE);
    p_hsl->saturation = MIN(p_hsl->saturation, 0xFE);

    //update brightness
    p_hsl->brightness = MIN(p_hsl->brightness, 0xFE);

    //update TT
    p_hsl->trans_time = (p_hsl->trans_time == 0xFFFF ? TT_DEFAULT : p_hsl->trans_time); 

    //do action
    kBulbStatus.task_cb = task_cb;
    lightDriver_setGlow(p_hsl, HSL, task_cb == NULL ? NULL : task_updateStatus_cb);
}
