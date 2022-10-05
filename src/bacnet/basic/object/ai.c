/**************************************************************************
 *
 * Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
 * Copyright (C) 2011 Krzysztof Malorny <malornykrzysztof@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *********************************************************************/

/* Analog Input Objects customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacenum.h"
#include "bacnet/bactext.h"
#include "bacnet/config.h" /* the custom stuff */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/proplist.h"
#include "bacnet/timestamp.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/sys/keylist.h"

#if PRINT_ENABLED
#include <stdio.h>
#define PRINTF(...) fprintf(stderr, __VA_ARGS__)
#else
#define PRINTF(...)
#endif

//#ifndef MAX_ANALOG_INPUTS
//#define MAX_ANALOG_INPUTS 4
//#endif

//! Custom property specific for Analog Input object type
//------------------------------------------------------------------------------
#define CUST_PROP_RXTIME                                                    9997
#define CUST_PROP_RXDATE                                                    9998
//! [add more custom properies]
//------------------------------------------------------------------------------
struct analog_input_descr{
    unsigned Event_State:3;
    float Present_Value;
    BACNET_RELIABILITY Reliability;
    bool Out_Of_Service;
    uint8_t Units;
    float Prior_Value;
    float COV_Increment;
    bool Changed;
    char* Object_Name;
    BACNET_TIME* rxTime;
    BACNET_DATE* rxDate;
    int16_t utcOffset;
#if defined(INTRINSIC_REPORTING)
    uint32_t Time_Delay;
    uint32_t Notification_Class;
    float High_Limit;
    float Low_Limit;
    float Deadband;
    unsigned Limit_Enable:2;
    unsigned Event_Enable:3;
    unsigned Notify_Type:1;
    ACKED_INFO Acked_Transitions[MAX_BACNET_EVENT_TRANSITION];
    BACNET_DATE_TIME Event_Time_Stamps[MAX_BACNET_EVENT_TRANSITION];
    /* time to generate event notification */
    uint32_t Remaining_Time_Delay;
    /* AckNotification information */
    ACK_NOTIFICATION Ack_notify_data;
#endif
};

//static ANALOG_INPUT_DESCR AI_Descr[MAX_ANALOG_INPUTS];

/* Key List for storing the object data sorted by instance number  */
static OS_Keylist Object_List;
//------------------------------------------------------------------------------
/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME, PROP_OBJECT_TYPE, PROP_PRESENT_VALUE, PROP_STATUS_FLAGS,
    PROP_EVENT_STATE, PROP_OUT_OF_SERVICE, PROP_UNITS, -1 };
//------------------------------------------------------------------------------
static const int Properties_Optional[] = { PROP_DESCRIPTION, PROP_RELIABILITY,
    PROP_COV_INCREMENT,
#if defined(INTRINSIC_REPORTING)
    PROP_TIME_DELAY, PROP_NOTIFICATION_CLASS, PROP_HIGH_LIMIT, PROP_LOW_LIMIT,
    PROP_DEADBAND, PROP_LIMIT_ENABLE, PROP_EVENT_ENABLE, PROP_ACKED_TRANSITIONS,
    PROP_NOTIFY_TYPE, PROP_EVENT_TIME_STAMPS,
#endif
    -1 };
//------------------------------------------------------------------------------
//static const int Properties_Proprietary[] = { 9997, 9998, 9999, -1 };
static const int Properties_Proprietary[] = {
                                                CUST_PROP_RXTIME,
                                                CUST_PROP_RXDATE, -1};
//------------------------------------------------------------------------------
void Analog_Input_Property_Lists(const int **pRequired, const int **pOptional,
                                                      const int **pProprietary){
    if(pRequired)
        *pRequired      = Properties_Required;
    if(pOptional)
        *pOptional      = Properties_Optional;
    if(pProprietary)
        *pProprietary   = Properties_Proprietary;
    return;
}
//------------------------------------------------------------------------------
void Analog_Input_Init(void){
//     unsigned i;
// #if defined(INTRINSIC_REPORTING)
//     unsigned j;
// #endif

//     for (i = 0; i < MAX_ANALOG_INPUTS; i++) {
//         AI_Descr[i].Present_Value = 0.0f;
//         AI_Descr[i].Out_Of_Service = false;
//         AI_Descr[i].Units = UNITS_PERCENT;
//         AI_Descr[i].Reliability = RELIABILITY_NO_FAULT_DETECTED;
//         AI_Descr[i].Prior_Value = 0.0f;
//         AI_Descr[i].COV_Increment = 1.0f;
//         AI_Descr[i].Changed = false;
// #if defined(INTRINSIC_REPORTING)
//         AI_Descr[i].Event_State = EVENT_STATE_NORMAL;
//         /* notification class not connected */
//         AI_Descr[i].Notification_Class = BACNET_MAX_INSTANCE;
//         /* initialize Event time stamps using wildcards
//            and set Acked_transitions */
//         for (j = 0; j < MAX_BACNET_EVENT_TRANSITION; j++) {
//             datetime_wildcard_set(&AI_Descr[i].Event_Time_Stamps[j]);
//             AI_Descr[i].Acked_Transitions[j].bIsAcked = true;
//         }

//         /* Set handler for GetEventInformation function */
//         handler_get_event_information_set(
//             OBJECT_ANALOG_INPUT, Analog_Input_Event_Information);
//         /* Set handler for AcknowledgeAlarm function */
//         handler_alarm_ack_set(OBJECT_ANALOG_INPUT, Analog_Input_Alarm_Ack);
//         /* Set handler for GetAlarmSummary Service */
//         handler_get_alarm_summary_set(
//             OBJECT_ANALOG_INPUT, Analog_Input_Alarm_Summary);
// #endif
//     }

    if(Object_List)
        Analog_Input_Cleanup();

    Object_List = Keylist_Create();
    if(Object_List){
        atexit(Analog_Input_Cleanup);
        /*
        for(unsigned i = 0; i < MAX_ANALOG_INPUTS; i++){
            if(Analog_Input_Create(i)){
                char str[32];
                sprintf(str, "Analog Input(%d)", i);
                Analog_Input_Name_Set(i, str);
            }
        }
        */
    }
}
//------------------------------------------------------------------------------
void Analog_Input_Cleanup(void){
    struct analog_input_descr* pObject;

    if(Object_List){
        do{
            pObject = (struct analog_input_descr*)Keylist_Data_Pop(Object_List);
            if(pObject){
                int i;
                //! Clear all objects allocated in the Object_List
                int objCount = Keylist_Count(Object_List);
                for(i=objCount-1;0<=i;i--){
                    Analog_Input_Delete(i);
                }
                free(pObject);
                Device_Inc_Database_Revision();
            }
        }while(pObject);
        Keylist_Delete(Object_List);
        Object_List = NULL;
    }
    printf("Analog_Input_Cleanup() was called");
}
//------------------------------------------------------------------------------
/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Analog_Input_Valid_Instance(uint32_t object_instance){
    unsigned int index = Analog_Input_Instance_To_Index(object_instance);
    unsigned int count = Analog_Input_Count();
    if(index < count)
        return true;
    return false;
}
//------------------------------------------------------------------------------
/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Analog_Input_Count(void){
    //return MAX_ANALOG_INPUTS;
    unsigned count = 0;
    if(Object_List)
        count = Keylist_Count(Object_List);
    return count;
}
//------------------------------------------------------------------------------
/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Analog_Input_Index_To_Instance(unsigned index){
    return index;
}
//------------------------------------------------------------------------------
/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Analog_Input_Instance_To_Index(uint32_t object_instance){
    unsigned index = Analog_Input_Count();
    if(object_instance < index)
        index = object_instance;
    return index;
}
//------------------------------------------------------------------------------
float Analog_Input_Present_Value(uint32_t object_instance){
    float value = 0.0;
    unsigned int index;
    if(Analog_Input_Valid_Instance(object_instance)){
        //value = AI_Descr[index].Present_Value;
        index = Analog_Input_Instance_To_Index(object_instance);
        struct analog_input_descr* pObject = (struct analog_input_descr*)
                                               Keylist_Data(Object_List, index);
        if(pObject){
            value = pObject->Present_Value;
        }
    }
    return value;
}
//------------------------------------------------------------------------------
static void Analog_Input_COV_Detect(unsigned int index, float value){
    float prior_value = 0.0;
    float cov_increment = 0.0;
    float cov_delta = 0.0;

    struct analog_input_descr* pObject = (struct analog_input_descr*)
                                               Keylist_Data(Object_List, index);
    if(pObject){
        prior_value     = pObject->Prior_Value;
        cov_increment   = pObject->COV_Increment;
        if(prior_value > value)
            cov_delta = prior_value - value;
        else
            cov_delta = value - prior_value;
        
        if(cov_delta >= cov_increment){
            pObject->Changed = true;
            pObject->Prior_Value = value;
        }
    }
}
//------------------------------------------------------------------------------
void Analog_Input_Present_Value_Set(uint32_t object_instance,  float value,
                                                                 time_t rxTime){
    unsigned int index = 0;

    if(Analog_Input_Valid_Instance(object_instance)){
        index = Analog_Input_Instance_To_Index(object_instance);
        struct analog_input_descr* pObject = (struct analog_input_descr*)
                                               Keylist_Data(Object_List, index);
        if(pObject){
            Analog_Input_COV_Detect(index, value);
            pObject->Present_Value = value;
            //! Also update rx time
            if(!pObject->rxTime)
                //! Allocate memory for the time structure before value
                //! assignment
                pObject->rxTime = (BACNET_TIME*)malloc(
                                                      sizeof(BACNET_TIME));
            if(!pObject->rxDate)
                pObject->rxDate = (BACNET_DATE*)malloc(
                                                      sizeof(BACNET_DATE));
            if(pObject->rxTime && pObject->rxDate){
                //! In case if local time must be show in the Object proeprties
                BACNET_DATE_TIME utcTime;
                uint64_t tmpRxTime = rxTime;
                datetime_since_epoch_seconds(&utcTime, tmpRxTime);

                //! Use UTC time
                pObject->rxDate->year          = utcTime.date.year;
                pObject->rxDate->month         = utcTime.date.month;
                pObject->rxDate->day           = utcTime.date.day;
                pObject->rxDate->wday          = utcTime.date.wday;
                pObject->rxTime->hour          = utcTime.time.hour;
                pObject->rxTime->min           = utcTime.time.min;
                pObject->rxTime->sec           = utcTime.time.sec;
                pObject->rxTime->hundredths    = utcTime.time.hundredths;

                //! ...or use local time (instead of UTC)
                /*
                bool dst = false;
                int16_t utcOffset = 0;
                BACNET_TIME localTime;
                BACNET_DATE date;
                BACNET_DATE_TIME localDT;
                datetime_local(&date, &localTime, &utcOffset, &dst);
                localDT.date = date;
                localDT.time = localTime;
                
                datetime_utc_to_local(&localDT, &utcTime, pObject->utcOffset, 0);
                pObject->rxDate->year          = localDT.date.year;
                pObject->rxDate->month         = localDT.date.month;
                pObject->rxDate->day           = localDT.date.day;
                pObject->rxDate->wday          = localDT.date.wday;

                pObject->rxTime->hour          = localDT.time.hour;
                pObject->rxTime->min           = localDT.time.min;
                pObject->rxTime->sec           = localDT.time.sec;
                pObject->rxTime->hundredths    = localDT.time.hundredths;
                */
            }
        }
    }
}
//------------------------------------------------------------------------------
bool Analog_Input_Object_Name(uint32_t object_instance,
                                          BACNET_CHARACTER_STRING *object_name){
    bool status = false;
    unsigned int index;
    if(Analog_Input_Valid_Instance(object_instance)){
        index = Analog_Input_Instance_To_Index(object_instance);
        struct analog_input_descr* pObject = (struct analog_input_descr*)
                                               Keylist_Data(Object_List, index);
        if(pObject)
            status = characterstring_init_ansi(object_name,
                                                          pObject->Object_Name);
    }
    return status;
}
//------------------------------------------------------------------------------
bool Analog_Input_Name_Set(uint32_t object_instance, const char* new_name){
    bool status = false;
    struct analog_input_descr *pObject = NULL;
    BACNET_CHARACTER_STRING object_name;
    BACNET_OBJECT_TYPE found_type = OBJECT_NONE;
    uint32_t found_instance = 0;
    unsigned int index;

    if(Analog_Input_Valid_Instance(object_instance)){
        index = Analog_Input_Instance_To_Index(object_instance);
        pObject = (struct analog_input_descr*)Keylist_Data(Object_List, index);
    }
    if(pObject){
        size_t str_len = strlen(new_name);
        if(new_name && str_len>0){
            characterstring_init_ansi(&object_name, new_name);
            if(Device_Valid_Object_Name(&object_name, &found_type, &found_instance)){
                if((found_type == OBJECT_ANALOG_INPUT) && (found_instance == object_instance)){
                    //! writing same name to same object
                    status = true;
                }
                else{
                    //! duplicate name
                    status = false;
                }
            }
            else{
                status = true;
                if(pObject->Object_Name){
                    //! realloc
                    pObject->Object_Name = (char*)realloc((void*)pObject->Object_Name, str_len+1);
                    snprintf(pObject->Object_Name, str_len+1, "%s", new_name);
                }
                else{
                    //! malloc
                    pObject->Object_Name = (char*)malloc(str_len+1);
                    snprintf(pObject->Object_Name, str_len+1, "%s", new_name);
                }
                Device_Inc_Database_Revision();
            }
        }
    }
    return status;
}
//------------------------------------------------------------------------------
/**
 * For a given object instance-number, gets the event-state property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  event-state property value
 */
unsigned Analog_Input_Event_State(uint32_t object_instance){
    unsigned state = EVENT_STATE_NORMAL;
#if defined(INTRINSIC_REPORTING)
    unsigned index = 0;
    if(Analog_Input_Valid_Instance(object_instance)){
        index = Analog_Input_Instance_To_Index(object_instance);
        struct analog_input_descr* pObject = (struct analog_input_descr*)
                                               Keylist_Data(Object_List, index);
        if(pObject)
            state = pObject->Event_State;
    }
#endif
    return state;
}
//------------------------------------------------------------------------------
bool Analog_Input_Change_Of_Value(uint32_t object_instance){
    unsigned index = 0;
    bool changed = false;

    if(Analog_Input_Valid_Instance(object_instance)){
        index = Analog_Input_Instance_To_Index(object_instance);
        struct analog_input_descr* pObject = (struct analog_input_descr*)
                                               Keylist_Data(Object_List, index);
        if(pObject)
            changed = pObject->Changed;
    }
    return changed;
}
//------------------------------------------------------------------------------
void Analog_Input_Change_Of_Value_Clear(uint32_t object_instance){
    unsigned index = 0;
    if(Analog_Input_Valid_Instance(object_instance)){
        index = Analog_Input_Instance_To_Index(object_instance);
        struct analog_input_descr* pObject = (struct analog_input_descr*)
                                               Keylist_Data(Object_List, index);
        if(pObject)
            pObject->Changed = false;
    }
}
//------------------------------------------------------------------------------
/**
 * For a given object instance-number, loads the value_list with the COV data.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value_list - list of COV data
 *
 * @return  true if the value list is encoded
 */
bool Analog_Input_Encode_Value_List(uint32_t object_instance,
                                             BACNET_PROPERTY_VALUE *value_list){
    bool status = false;
    bool in_alarm = false;
    bool out_of_service = false;
    const bool fault = false;
    const bool overridden = false;
    float present_value = 0.0;

    if(Analog_Input_Valid_Instance(object_instance)){
        unsigned index = Analog_Input_Instance_To_Index(object_instance);
        struct analog_input_descr* pObject = (struct analog_input_descr*)
                                               Keylist_Data(Object_List, index);
        if(pObject){
            if(pObject->Event_State != EVENT_STATE_NORMAL){
                in_alarm = true;
            }
            out_of_service = pObject->Out_Of_Service;
            present_value = pObject->Present_Value;
            status = cov_value_list_encode_real(value_list, present_value,
                                   in_alarm, fault, overridden, out_of_service);
        }
    }
    return status;
}
//------------------------------------------------------------------------------
float Analog_Input_COV_Increment(uint32_t object_instance){
    float value = 0;
    if(Analog_Input_Valid_Instance(object_instance)){
        unsigned index = Analog_Input_Instance_To_Index(object_instance);
        struct analog_input_descr* pObject = (struct analog_input_descr*)
                                               Keylist_Data(Object_List, index);
        if(pObject)
            value = pObject->COV_Increment;
    }
    return value;
}
//------------------------------------------------------------------------------
void Analog_Input_COV_Increment_Set(uint32_t object_instance, float value){
    if(Analog_Input_Valid_Instance(object_instance)){
        unsigned index = Analog_Input_Instance_To_Index(object_instance);
        struct analog_input_descr* pObject = (struct analog_input_descr*)
                                               Keylist_Data(Object_List, index);
        if(pObject){
            pObject->COV_Increment = value;
            Analog_Input_COV_Detect(index, pObject->Present_Value);
        }
    }
}
//------------------------------------------------------------------------------
bool Analog_Input_Out_Of_Service(uint32_t object_instance){
    bool value = false;
    if(Analog_Input_Valid_Instance(object_instance)){
        unsigned index = Analog_Input_Instance_To_Index(object_instance);
        struct analog_input_descr* pObject = (struct analog_input_descr*)
                                               Keylist_Data(Object_List, index);
        if(pObject)
            value = pObject->Out_Of_Service;
    }
    return value;
}
//------------------------------------------------------------------------------
void Analog_Input_Out_Of_Service_Set(uint32_t object_instance, bool value){
    if(Analog_Input_Valid_Instance(object_instance)){
        unsigned index = Analog_Input_Instance_To_Index(object_instance);
        struct analog_input_descr* pObject = (struct analog_input_descr*)
                                               Keylist_Data(Object_List, index);
        if(pObject){
            /* 	BACnet Testing Observed Incident oi00104
            The Changed flag was not being set when a client wrote to the
            Out-of-Service bit. Revealed by BACnet Test Client v1.8.16 (
            www.bac-test.com/bacnet-test-client-download ) BC 135.1: 8.2.1-A BC
            135.1: 8.2.2-A Any discussions can be directed to edward@bac-test.com
            Please feel free to remove this comment when my changes accepted after
            suitable time for review by all interested parties. Say 6 months ->
            September 2016 */
            if(pObject->Out_Of_Service != value)
                pObject->Changed = true;
            pObject->Out_Of_Service = value;
        }
    }
}
//------------------------------------------------------------------------------
bool Analog_Input_Units_Set(uint32_t object_instance,
                                               BACNET_ENGINEERING_UNITS unitID){
    bool status = false;
    if(Analog_Input_Valid_Instance(object_instance)){
        unsigned index = Analog_Input_Instance_To_Index(object_instance);
        struct analog_input_descr* pObject = (struct analog_input_descr*)
                                               Keylist_Data(Object_List, index);
        if(pObject){
            pObject->Units = unitID;
            status = true;
        }
    }
    return status;
}
//------------------------------------------------------------------------------
void Analog_Input_UTCOffset_Set(uint32_t object_instance, int16_t _utcOffset){
    if(Analog_Input_Valid_Instance(object_instance)){
        unsigned index = Analog_Input_Instance_To_Index(object_instance);
        struct analog_input_descr* pObject = (struct analog_input_descr*)
                                               Keylist_Data(Object_List, index);
        if(pObject){
            pObject->utcOffset = _utcOffset;
        }
    }
}
//------------------------------------------------------------------------------
/* return apdu length, or BACNET_STATUS_ERROR on error */
/* assumption - object already exists */
int Analog_Input_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata){
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    ANALOG_INPUT_DESCR *CurrentAI;
    unsigned object_index = 0;
#if defined(INTRINSIC_REPORTING)
    unsigned i = 0;
    int len = 0;
#endif
    uint8_t *apdu = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }

    if(Analog_Input_Valid_Instance(rpdata->object_instance)){
        object_index = Analog_Input_Instance_To_Index(rpdata->object_instance);
        CurrentAI = (struct analog_input_descr*)Keylist_Data(Object_List,
                                                                  object_index);
        if(CurrentAI==NULL)
            return BACNET_STATUS_ERROR;
    }
    else
        return BACNET_STATUS_ERROR;

    apdu = rpdata->application_data;
    switch ((int)rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_ANALOG_INPUT, rpdata->object_instance);
            break;

        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            Analog_Input_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;

        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_ANALOG_INPUT);
            break;

        case PROP_PRESENT_VALUE:
            apdu_len = encode_application_real(
                &apdu[0], Analog_Input_Present_Value(rpdata->object_instance));
            break;

        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM,
                Analog_Input_Event_State(rpdata->object_instance) !=
                EVENT_STATE_NORMAL);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE,
                CurrentAI->Out_Of_Service);

            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;

        case PROP_EVENT_STATE:
            apdu_len =
                encode_application_enumerated(&apdu[0],
                Analog_Input_Event_State(rpdata->object_instance));
            break;

        case PROP_RELIABILITY:
            apdu_len =
                encode_application_enumerated(&apdu[0], CurrentAI->Reliability);
            break;

        case PROP_OUT_OF_SERVICE:
            apdu_len =
                encode_application_boolean(&apdu[0], CurrentAI->Out_Of_Service);
            break;

        case PROP_UNITS:
            apdu_len =
                encode_application_enumerated(&apdu[0], CurrentAI->Units);
            break;

        case PROP_COV_INCREMENT:
            apdu_len =
                encode_application_real(&apdu[0], CurrentAI->COV_Increment);
            break;

#if defined(INTRINSIC_REPORTING)
        case PROP_TIME_DELAY:
            apdu_len =
                encode_application_unsigned(&apdu[0], CurrentAI->Time_Delay);
            break;

        case PROP_NOTIFICATION_CLASS:
            apdu_len = encode_application_unsigned(
                &apdu[0], CurrentAI->Notification_Class);
            break;

        case PROP_HIGH_LIMIT:
            apdu_len = encode_application_real(&apdu[0], CurrentAI->High_Limit);
            break;

        case PROP_LOW_LIMIT:
            apdu_len = encode_application_real(&apdu[0], CurrentAI->Low_Limit);
            break;

        case PROP_DEADBAND:
            apdu_len = encode_application_real(&apdu[0], CurrentAI->Deadband);
            break;

        case PROP_LIMIT_ENABLE:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, 0,
                (CurrentAI->Limit_Enable & EVENT_LOW_LIMIT_ENABLE) ? true
                                                                   : false);
            bitstring_set_bit(&bit_string, 1,
                (CurrentAI->Limit_Enable & EVENT_HIGH_LIMIT_ENABLE) ? true
                                                                    : false);

            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;

        case PROP_EVENT_ENABLE:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, TRANSITION_TO_OFFNORMAL,
                (CurrentAI->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) ? true
                                                                      : false);
            bitstring_set_bit(&bit_string, TRANSITION_TO_FAULT,
                (CurrentAI->Event_Enable & EVENT_ENABLE_TO_FAULT) ? true
                                                                  : false);
            bitstring_set_bit(&bit_string, TRANSITION_TO_NORMAL,
                (CurrentAI->Event_Enable & EVENT_ENABLE_TO_NORMAL) ? true
                                                                   : false);

            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;

        case PROP_ACKED_TRANSITIONS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, TRANSITION_TO_OFFNORMAL,
                CurrentAI->Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked);
            bitstring_set_bit(&bit_string, TRANSITION_TO_FAULT,
                CurrentAI->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked);
            bitstring_set_bit(&bit_string, TRANSITION_TO_NORMAL,
                CurrentAI->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked);

            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;

        case PROP_NOTIFY_TYPE:
            apdu_len = encode_application_enumerated(
                &apdu[0], CurrentAI->Notify_Type ? NOTIFY_EVENT : NOTIFY_ALARM);
            break;

        case PROP_EVENT_TIME_STAMPS:
            /* Array element zero is the number of elements in the array */
            if (rpdata->array_index == 0)
                apdu_len = encode_application_unsigned(
                    &apdu[0], MAX_BACNET_EVENT_TRANSITION);
            /* if no index was specified, then try to encode the entire list */
            /* into one packet. */
            else if (rpdata->array_index == BACNET_ARRAY_ALL) {
                for (i = 0; i < MAX_BACNET_EVENT_TRANSITION; i++) {
                    len = encode_opening_tag(
                        &apdu[apdu_len], TIME_STAMP_DATETIME);
                    len += encode_application_date(&apdu[apdu_len + len],
                        &CurrentAI->Event_Time_Stamps[i].date);
                    len += encode_application_time(&apdu[apdu_len + len],
                        &CurrentAI->Event_Time_Stamps[i].time);
                    len += encode_closing_tag(
                        &apdu[apdu_len + len], TIME_STAMP_DATETIME);

                    /* add it if we have room */
                    if ((apdu_len + len) < MAX_APDU)
                        apdu_len += len;
                    else {
                        rpdata->error_code =
                            ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
                        apdu_len = BACNET_STATUS_ABORT;
                        break;
                    }
                }
            } else if (rpdata->array_index <= MAX_BACNET_EVENT_TRANSITION) {
                apdu_len =
                    encode_opening_tag(&apdu[apdu_len], TIME_STAMP_DATETIME);
                apdu_len += encode_application_date(&apdu[apdu_len],
                    &CurrentAI->Event_Time_Stamps[rpdata->array_index].date);
                apdu_len += encode_application_time(&apdu[apdu_len],
                    &CurrentAI->Event_Time_Stamps[rpdata->array_index].time);
                apdu_len +=
                    encode_closing_tag(&apdu[apdu_len], TIME_STAMP_DATETIME);
            } else {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
        // case 9997:
        //     /* test case for real encoding-decoding real value correctly */
        //     apdu_len = encode_application_real(&apdu[0], 90.510F);
        //     break;
        // case 9998:
        //     /* test case for unsigned encoding-decoding unsigned value correctly
        //      */
        //     apdu_len = encode_application_unsigned(&apdu[0], 90);
        //     break;
        // case 9999:
        //     /* test case for signed encoding-decoding negative value correctly
        //      */
        //     apdu_len = encode_application_signed(&apdu[0], -200);
        //     break;
        case CUST_PROP_RXTIME:{
            if(CurrentAI->rxTime)
                apdu_len = encode_application_time(&apdu[0], CurrentAI->rxTime);
            else{
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_VALUE_NOT_INITIALIZED;
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
        }
        case CUST_PROP_RXDATE:{
            if(CurrentAI->rxDate)
                apdu_len = encode_application_date(&apdu[0], CurrentAI->rxDate);
            else{
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_VALUE_NOT_INITIALIZED;
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
        }
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) &&
        (rpdata->object_property != PROP_EVENT_TIME_STAMPS) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}
//------------------------------------------------------------------------------
/* returns true if successful */
bool Analog_Input_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data){
    bool status = false; /* return value */
    unsigned int object_index = 0;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;
    ANALOG_INPUT_DESCR *CurrentAI;

    /* decode the some of the request */
    len = bacapp_decode_application_data(
        wp_data->application_data, wp_data->application_data_len, &value);
    /* FIXME: len < application_data_len: more data? */
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    /*  only array properties can have array options */
    if ((wp_data->object_property != PROP_EVENT_TIME_STAMPS) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }

    if(Analog_Input_Valid_Instance(wp_data->object_instance)){
        object_index = Analog_Input_Instance_To_Index(wp_data->object_instance);
        CurrentAI = (struct analog_input_descr*)Keylist_Data(Object_List,
                                                                  object_index);
        if(CurrentAI==NULL)
            return false;
    }
    else
        return false;

    switch ((int)wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            /*
            status = write_property_type_valid(wp_data, &value,
                BACNET_APPLICATION_TAG_REAL);
            if (status) {
                if (CurrentAI->Out_Of_Service == true) {
                    Analog_Input_Present_Value_Set(
                        wp_data->object_instance, value.type.Real);
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                    status = false;
                }
            }
            */
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(wp_data, &value,
                BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Analog_Input_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;

        case PROP_UNITS:
            status = write_property_type_valid(wp_data, &value,
                BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                CurrentAI->Units = value.type.Enumerated;
            }
            break;

        case PROP_COV_INCREMENT:
            status = write_property_type_valid(wp_data, &value,
                BACNET_APPLICATION_TAG_REAL);
            if (status) {
                if (value.type.Real >= 0.0) {
                    Analog_Input_COV_Increment_Set(
                        wp_data->object_instance, value.type.Real);
                } else {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;

#if defined(INTRINSIC_REPORTING)
        case PROP_TIME_DELAY:
            status = write_property_type_valid(wp_data, &value,
                BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                CurrentAI->Time_Delay = value.type.Unsigned_Int;
                CurrentAI->Remaining_Time_Delay = CurrentAI->Time_Delay;
            }
            break;

        case PROP_NOTIFICATION_CLASS:
            status = write_property_type_valid(wp_data, &value,
                BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                CurrentAI->Notification_Class = value.type.Unsigned_Int;
            }
            break;

        case PROP_HIGH_LIMIT:
            status = write_property_type_valid(wp_data, &value,
                BACNET_APPLICATION_TAG_REAL);
            if (status) {
                CurrentAI->High_Limit = value.type.Real;
            }
            break;

        case PROP_LOW_LIMIT:
            status = write_property_type_valid(wp_data, &value,
                BACNET_APPLICATION_TAG_REAL);
            if (status) {
                CurrentAI->Low_Limit = value.type.Real;
            }
            break;

        case PROP_DEADBAND:
            status = write_property_type_valid(wp_data, &value,
                BACNET_APPLICATION_TAG_REAL);
            if (status) {
                CurrentAI->Deadband = value.type.Real;
            }
            break;

        case PROP_LIMIT_ENABLE:
            status = write_property_type_valid(wp_data, &value,
                BACNET_APPLICATION_TAG_BIT_STRING);
            if (status) {
                if (value.type.Bit_String.bits_used == 2) {
                    CurrentAI->Limit_Enable = value.type.Bit_String.value[0];
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    status = false;
                }
            }
            break;

        case PROP_EVENT_ENABLE:
            status = write_property_type_valid(wp_data, &value,
                BACNET_APPLICATION_TAG_BIT_STRING);
            if (status) {
                if (value.type.Bit_String.bits_used == 3) {
                    CurrentAI->Event_Enable = value.type.Bit_String.value[0];
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    status = false;
                }
            }
            break;

        case PROP_NOTIFY_TYPE:
            status = write_property_type_valid(wp_data, &value,
                BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                switch ((BACNET_NOTIFY_TYPE)value.type.Enumerated) {
                    case NOTIFY_EVENT:
                        CurrentAI->Notify_Type = 1;
                        break;
                    case NOTIFY_ALARM:
                        CurrentAI->Notify_Type = 0;
                        break;
                    default:
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                        status = false;
                        break;
                }
            }
            break;
#endif
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_NAME:
        case PROP_OBJECT_TYPE:
        case PROP_STATUS_FLAGS:
        case PROP_EVENT_STATE:
        case PROP_DESCRIPTION:
        case PROP_RELIABILITY:
#if defined(INTRINSIC_REPORTING)
        case PROP_ACKED_TRANSITIONS:
        case PROP_EVENT_TIME_STAMPS:
#endif
        //case 9997:
        //case 9998:
        //case 9999:
        case CUST_PROP_RXTIME:
        case CUST_PROP_RXDATE:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            break;
    }

    return status;
}
//------------------------------------------------------------------------------
void Analog_Input_Intrinsic_Reporting(uint32_t object_instance){
#if defined(INTRINSIC_REPORTING)
    BACNET_EVENT_NOTIFICATION_DATA event_data = { 0 };
    BACNET_CHARACTER_STRING msgText = { 0 };
    ANALOG_INPUT_DESCR *CurrentAI = NULL;
    unsigned int object_index = 0;
    uint8_t FromState = 0;
    uint8_t ToState = 0;
    float ExceededLimit = 0.0f;
    float PresentVal = 0.0f;
    bool SendNotify = false;

    if(Analog_Input_Valid_Instance(object_instance)){
        object_index = Analog_Input_Instance_To_Index(object_instance);
        CurrentAI = (struct analog_input_descr*)Keylist_Data(Object_List,
                                                                  object_index);
        if(CurrentAI==NULL)
            return;
    }
    else
        return;

    /* check limits */
    if (!CurrentAI->Limit_Enable) {
        return; /* limits are not configured */
    }

    if (CurrentAI->Ack_notify_data.bSendAckNotify) {
        /* clean bSendAckNotify flag */
        CurrentAI->Ack_notify_data.bSendAckNotify = false;
        /* copy toState */
        ToState = CurrentAI->Ack_notify_data.EventState;
        PRINTF("Analog-Input[%d]: Send AckNotification.\n", object_instance);
        characterstring_init_ansi(&msgText, "AckNotification");

        /* Notify Type */
        event_data.notifyType = NOTIFY_ACK_NOTIFICATION;

        /* Send EventNotification. */
        SendNotify = true;
    } else {
        /* actual Present_Value */
        PresentVal = Analog_Input_Present_Value(object_instance);
        FromState = CurrentAI->Event_State;
        switch (CurrentAI->Event_State) {
            case EVENT_STATE_NORMAL:
                /* A TO-OFFNORMAL event is generated under these conditions:
                   (a) the Present_Value must exceed the High_Limit for a
                   minimum period of time, specified in the Time_Delay property,
                   and (b) the HighLimitEnable flag must be set in the
                   Limit_Enable property, and
                   (c) the TO-OFFNORMAL flag must be set in the Event_Enable
                   property. */
                if ((PresentVal > CurrentAI->High_Limit) &&
                    ((CurrentAI->Limit_Enable & EVENT_HIGH_LIMIT_ENABLE) ==
                        EVENT_HIGH_LIMIT_ENABLE) &&
                    ((CurrentAI->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) ==
                        EVENT_ENABLE_TO_OFFNORMAL)) {
                    if (!CurrentAI->Remaining_Time_Delay)
                        CurrentAI->Event_State = EVENT_STATE_HIGH_LIMIT;
                    else
                        CurrentAI->Remaining_Time_Delay--;
                    break;
                }

                /* A TO-OFFNORMAL event is generated under these conditions:
                   (a) the Present_Value must exceed the Low_Limit plus the
                   Deadband for a minimum period of time, specified in the
                   Time_Delay property, and (b) the LowLimitEnable flag must be
                   set in the Limit_Enable property, and
                   (c) the TO-NORMAL flag must be set in the Event_Enable
                   property. */
                if ((PresentVal < CurrentAI->Low_Limit) &&
                    ((CurrentAI->Limit_Enable & EVENT_LOW_LIMIT_ENABLE) ==
                        EVENT_LOW_LIMIT_ENABLE) &&
                    ((CurrentAI->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) ==
                        EVENT_ENABLE_TO_OFFNORMAL)) {
                    if (!CurrentAI->Remaining_Time_Delay)
                        CurrentAI->Event_State = EVENT_STATE_LOW_LIMIT;
                    else
                        CurrentAI->Remaining_Time_Delay--;
                    break;
                }
                /* value of the object is still in the same event state */
                CurrentAI->Remaining_Time_Delay = CurrentAI->Time_Delay;
                break;

            case EVENT_STATE_HIGH_LIMIT:
                /* Once exceeded, the Present_Value must fall below the
                   High_Limit minus the Deadband before a TO-NORMAL event is
                   generated under these conditions: (a) the Present_Value must
                   fall below the High_Limit minus the Deadband for a minimum
                   period of time, specified in the Time_Delay property, and (b)
                   the HighLimitEnable flag must be set in the Limit_Enable
                   property, and (c) the TO-NORMAL flag must be set in the
                   Event_Enable property. */
                if ((PresentVal <
                        CurrentAI->High_Limit - CurrentAI->Deadband) &&
                    ((CurrentAI->Limit_Enable & EVENT_HIGH_LIMIT_ENABLE) ==
                        EVENT_HIGH_LIMIT_ENABLE) &&
                    ((CurrentAI->Event_Enable & EVENT_ENABLE_TO_NORMAL) ==
                        EVENT_ENABLE_TO_NORMAL)) {
                    if (!CurrentAI->Remaining_Time_Delay)
                        CurrentAI->Event_State = EVENT_STATE_NORMAL;
                    else
                        CurrentAI->Remaining_Time_Delay--;
                    break;
                }
                /* value of the object is still in the same event state */
                CurrentAI->Remaining_Time_Delay = CurrentAI->Time_Delay;
                break;

            case EVENT_STATE_LOW_LIMIT:
                /* Once the Present_Value has fallen below the Low_Limit,
                   the Present_Value must exceed the Low_Limit plus the Deadband
                   before a TO-NORMAL event is generated under these conditions:
                   (a) the Present_Value must exceed the Low_Limit plus the
                   Deadband for a minimum period of time, specified in the
                   Time_Delay property, and (b) the LowLimitEnable flag must be
                   set in the Limit_Enable property, and
                   (c) the TO-NORMAL flag must be set in the Event_Enable
                   property. */
                if ((PresentVal > CurrentAI->Low_Limit + CurrentAI->Deadband) &&
                    ((CurrentAI->Limit_Enable & EVENT_LOW_LIMIT_ENABLE) ==
                        EVENT_LOW_LIMIT_ENABLE) &&
                    ((CurrentAI->Event_Enable & EVENT_ENABLE_TO_NORMAL) ==
                        EVENT_ENABLE_TO_NORMAL)) {
                    if (!CurrentAI->Remaining_Time_Delay)
                        CurrentAI->Event_State = EVENT_STATE_NORMAL;
                    else
                        CurrentAI->Remaining_Time_Delay--;
                    break;
                }
                /* value of the object is still in the same event state */
                CurrentAI->Remaining_Time_Delay = CurrentAI->Time_Delay;
                break;

            default:
                return; /* shouldn't happen */
        } /* switch (FromState) */

        ToState = CurrentAI->Event_State;

        if (FromState != ToState) {
            /* Event_State has changed.
               Need to fill only the basic parameters of this type of event.
               Other parameters will be filled in common function. */

            switch (ToState) {
                case EVENT_STATE_HIGH_LIMIT:
                    ExceededLimit = CurrentAI->High_Limit;
                    characterstring_init_ansi(&msgText, "Goes to high limit");
                    break;

                case EVENT_STATE_LOW_LIMIT:
                    ExceededLimit = CurrentAI->Low_Limit;
                    characterstring_init_ansi(&msgText, "Goes to low limit");
                    break;

                case EVENT_STATE_NORMAL:
                    if (FromState == EVENT_STATE_HIGH_LIMIT) {
                        ExceededLimit = CurrentAI->High_Limit;
                        characterstring_init_ansi(
                            &msgText, "Back to normal state from high limit");
                    } else {
                        ExceededLimit = CurrentAI->Low_Limit;
                        characterstring_init_ansi(
                            &msgText, "Back to normal state from low limit");
                    }
                    break;

                default:
                    ExceededLimit = 0;
                    break;
            } /* switch (ToState) */
            PRINTF("Analog-Input[%d]: Event_State goes from %s to %s.\n",
                object_instance,
                bactext_event_state_name(FromState),
                bactext_event_state_name(ToState));
            /* Notify Type */
            event_data.notifyType = CurrentAI->Notify_Type;

            /* Send EventNotification. */
            SendNotify = true;
        }
    }

    if (SendNotify) {
        /* Event Object Identifier */
        event_data.eventObjectIdentifier.type = OBJECT_ANALOG_INPUT;
        event_data.eventObjectIdentifier.instance = object_instance;

        /* Time Stamp */
        event_data.timeStamp.tag = TIME_STAMP_DATETIME;
        if (event_data.notifyType != NOTIFY_ACK_NOTIFICATION) {
            Device_getCurrentDateTime(&event_data.timeStamp.value.dateTime);
            /* fill Event_Time_Stamps */
            switch (ToState) {
                case EVENT_STATE_HIGH_LIMIT:
                case EVENT_STATE_LOW_LIMIT:
                    datetime_copy(
                        &CurrentAI->Event_Time_Stamps[TRANSITION_TO_OFFNORMAL],
                        &event_data.timeStamp.value.dateTime);
                    break;
                case EVENT_STATE_FAULT:
                    datetime_copy(
                        &CurrentAI->Event_Time_Stamps[TRANSITION_TO_FAULT],
                        &event_data.timeStamp.value.dateTime);
                    break;
                case EVENT_STATE_NORMAL:
                    datetime_copy(
                        &CurrentAI->Event_Time_Stamps[TRANSITION_TO_NORMAL],
                        &event_data.timeStamp.value.dateTime);
                    break;
                default:
                    break;
            }
        } else {
            /* fill event_data timeStamp */
            switch (ToState) {
                case EVENT_STATE_HIGH_LIMIT:
                case EVENT_STATE_LOW_LIMIT:
                    datetime_copy(
                        &event_data.timeStamp.value.dateTime,
                        &CurrentAI->Event_Time_Stamps[TRANSITION_TO_OFFNORMAL]);
                    break;
                case EVENT_STATE_FAULT:
                    datetime_copy(
                        &event_data.timeStamp.value.dateTime,
                        &CurrentAI->Event_Time_Stamps[TRANSITION_TO_FAULT]);
                    break;
                case EVENT_STATE_NORMAL:
                    datetime_copy(
                        &event_data.timeStamp.value.dateTime,
                        &CurrentAI->Event_Time_Stamps[TRANSITION_TO_NORMAL]);
                    break;
                default:
                    break;
            }
        }

        /* Notification Class */
        event_data.notificationClass = CurrentAI->Notification_Class;

        /* Event Type */
        event_data.eventType = EVENT_OUT_OF_RANGE;

        /* Message Text */
        event_data.messageText = &msgText;

        /* Notify Type */
        /* filled before */

        /* From State */
        if (event_data.notifyType != NOTIFY_ACK_NOTIFICATION)
            event_data.fromState = FromState;

        /* To State */
        event_data.toState = CurrentAI->Event_State;

        /* Event Values */
        if (event_data.notifyType != NOTIFY_ACK_NOTIFICATION) {
            //! Value that exceeded a limit
            event_data.notificationParams.outOfRange.exceedingValue =
                PresentVal;
            //! Status_Flags of the referenced object
            bitstring_init(
                &event_data.notificationParams.outOfRange.statusFlags);
            bitstring_set_bit(
                &event_data.notificationParams.outOfRange.statusFlags,
                STATUS_FLAG_IN_ALARM,
                CurrentAI->Event_State != EVENT_STATE_NORMAL);
            bitstring_set_bit(
                &event_data.notificationParams.outOfRange.statusFlags,
                STATUS_FLAG_FAULT, false);
            bitstring_set_bit(
                &event_data.notificationParams.outOfRange.statusFlags,
                STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(
                &event_data.notificationParams.outOfRange.statusFlags,
                STATUS_FLAG_OUT_OF_SERVICE, CurrentAI->Out_Of_Service);
            //! Deadband used for limit checking
            event_data.notificationParams.outOfRange.deadband =
                CurrentAI->Deadband;
            //! Limit that was exceeded
            event_data.notificationParams.outOfRange.exceededLimit =
                ExceededLimit;
        }

        //! Add data from notification class
        PRINTF("Analog-Input[%d]: Notification Class[%d]-%s "
                "%u/%u/%u-%u:%u:%u.%u!\n",
                object_instance, event_data.notificationClass,
                bactext_event_type_name(event_data.eventType),
                (unsigned)event_data.timeStamp.value.dateTime.date.year,
                (unsigned)event_data.timeStamp.value.dateTime.date.month,
                (unsigned)event_data.timeStamp.value.dateTime.date.day,
                (unsigned)event_data.timeStamp.value.dateTime.time.hour,
                (unsigned)event_data.timeStamp.value.dateTime.time.min,
                (unsigned)event_data.timeStamp.value.dateTime.time.sec,
                (unsigned)event_data.timeStamp.value.dateTime.time.hundredths);
        Notification_Class_common_reporting_function(&event_data);

        //! Ack required
        if ((event_data.notifyType != NOTIFY_ACK_NOTIFICATION) &&
            (event_data.ackRequired == true)) {
            PRINTF("Analog-Input[%d]: Ack Required!\n", object_instance);
            switch (event_data.toState) {
                case EVENT_STATE_OFFNORMAL:
                case EVENT_STATE_HIGH_LIMIT:
                case EVENT_STATE_LOW_LIMIT:
                    CurrentAI->Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                        .bIsAcked = false;
                    CurrentAI->Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                        .Time_Stamp = event_data.timeStamp.value.dateTime;
                    break;

                case EVENT_STATE_FAULT:
                    CurrentAI->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked =
                        false;
                    CurrentAI->Acked_Transitions[TRANSITION_TO_FAULT]
                        .Time_Stamp = event_data.timeStamp.value.dateTime;
                    break;

                case EVENT_STATE_NORMAL:
                    CurrentAI->Acked_Transitions[TRANSITION_TO_NORMAL]
                        .bIsAcked = false;
                    CurrentAI->Acked_Transitions[TRANSITION_TO_NORMAL]
                        .Time_Stamp = event_data.timeStamp.value.dateTime;
                    break;

                default: // shouldn't happen
                    break;
            }
        }
    }
//! defined(INTRINSIC_REPORTING)
#endif
}
//------------------------------------------------------------------------------
#if defined(INTRINSIC_REPORTING)
int Analog_Input_Event_Information(unsigned index,
                              BACNET_GET_EVENT_INFORMATION_DATA *getevent_data){
    bool IsNotAckedTransitions;
    bool IsActiveEvent;

    ANALOG_INPUT_DESCR *CurrentAI = NULL;
    CurrentAI = (struct analog_input_descr*)Keylist_Data(Object_List, index);
    if(CurrentAI==NULL)
        //! End of list
        return -1;
    else{
        //! Event_State not equal to NORMAL
        IsActiveEvent = (CurrentAI->Event_State != EVENT_STATE_NORMAL);
        //! Acked_Transitions property, which has at least one of the bits
        //! (TO-OFFNORMAL, TO-FAULT, TONORMAL) set to FALSE.
        IsNotAckedTransitions =
            (CurrentAI->Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked
                                                                     == false) |
            (CurrentAI->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked
                                                                     == false) |
            (CurrentAI->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked
                                                                     == false);
    }

    if ((IsActiveEvent) || (IsNotAckedTransitions)) {
        //! Object Identifier
        getevent_data->objectIdentifier.type = OBJECT_ANALOG_INPUT;
        getevent_data->objectIdentifier.instance =
            Analog_Input_Index_To_Instance(index);
        //! Event State
        getevent_data->eventState = CurrentAI->Event_State;
        //! Acknowledged Transitions
        bitstring_init(&getevent_data->acknowledgedTransitions);
        bitstring_set_bit(&getevent_data->acknowledgedTransitions,
            TRANSITION_TO_OFFNORMAL,
            CurrentAI->Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked);
        bitstring_set_bit(&getevent_data->acknowledgedTransitions,
            TRANSITION_TO_FAULT,
            CurrentAI->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked);
        bitstring_set_bit(&getevent_data->acknowledgedTransitions,
            TRANSITION_TO_NORMAL,
            CurrentAI->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked);
        //! Event Time Stamps
        int i;
        for (i = 0; i < 3; i++) {
            getevent_data->eventTimeStamps[i].tag = TIME_STAMP_DATETIME;
            getevent_data->eventTimeStamps[i].value.dateTime =
                CurrentAI->Event_Time_Stamps[i];
        }
        //! Notify Type
        getevent_data->notifyType = CurrentAI->Notify_Type;
        //! Event Enable
        bitstring_init(&getevent_data->eventEnable);
        bitstring_set_bit(&getevent_data->eventEnable, TRANSITION_TO_OFFNORMAL,
            (CurrentAI->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) ? true
                                                                       : false);
        bitstring_set_bit(&getevent_data->eventEnable, TRANSITION_TO_FAULT,
            (CurrentAI->Event_Enable & EVENT_ENABLE_TO_FAULT) ? true
                                                                   : false);
        bitstring_set_bit(&getevent_data->eventEnable, TRANSITION_TO_NORMAL,
            (CurrentAI->Event_Enable & EVENT_ENABLE_TO_NORMAL) ? true
                                                                    : false);
        //! Event Priorities
        Notification_Class_Get_Priorities(
            CurrentAI->Notification_Class, getevent_data->eventPriorities);

        //! Active event
        return 1;
    }
    else
        //! No active event at this index
        return 0;
}
//------------------------------------------------------------------------------
int Analog_Input_Alarm_Ack(BACNET_ALARM_ACK_DATA *alarmack_data,
                                                 BACNET_ERROR_CODE *error_code){
    ANALOG_INPUT_DESCR *CurrentAI;
    unsigned int object_index;
    unsigned int tmp_instance = alarmack_data->eventObjectIdentifier.instance;
    if(Analog_Input_Valid_Instance(tmp_instance)){
        object_index = Analog_Input_Instance_To_Index(tmp_instance);
        CurrentAI = (struct analog_input_descr*)Keylist_Data(Object_List,
                                                                  object_index);
        if(CurrentAI==NULL){
            *error_code = ERROR_CODE_UNKNOWN_OBJECT;
             return -1;
        }
    }
    else{
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return -1;
    }

    switch (alarmack_data->eventStateAcked) {
        case EVENT_STATE_OFFNORMAL:
        case EVENT_STATE_HIGH_LIMIT:
        case EVENT_STATE_LOW_LIMIT:
            if (CurrentAI->Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                    .bIsAcked == false) {
                if (alarmack_data->eventTimeStamp.tag != TIME_STAMP_DATETIME) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                if (datetime_compare(
                        &CurrentAI->Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                             .Time_Stamp,
                        &alarmack_data->eventTimeStamp.value.dateTime) > 0) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                //! Send ack notification
                CurrentAI->Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked =
                    true;
            } else if (alarmack_data->eventStateAcked ==
                CurrentAI->Event_State) {
                //! Send ack notification
            } else {
                *error_code = ERROR_CODE_INVALID_EVENT_STATE;
                return -1;
            }
            break;

        case EVENT_STATE_FAULT:
            if (CurrentAI->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked ==
                false) {
                if (alarmack_data->eventTimeStamp.tag != TIME_STAMP_DATETIME) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                if (datetime_compare(
                        &CurrentAI->Acked_Transitions[TRANSITION_TO_FAULT]
                             .Time_Stamp,
                        &alarmack_data->eventTimeStamp.value.dateTime) > 0) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                //! Send ack notification
                CurrentAI->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked =
                    true;
            } else if (alarmack_data->eventStateAcked ==
                CurrentAI->Event_State) {
                //! Send ack notification
            } else {
                *error_code = ERROR_CODE_INVALID_EVENT_STATE;
                return -1;
            }
            break;

        case EVENT_STATE_NORMAL:
            if (CurrentAI->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked ==
                false) {
                if (alarmack_data->eventTimeStamp.tag != TIME_STAMP_DATETIME) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                if (datetime_compare(
                        &CurrentAI->Acked_Transitions[TRANSITION_TO_NORMAL]
                             .Time_Stamp,
                        &alarmack_data->eventTimeStamp.value.dateTime) > 0) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                //! Send ack notification
                CurrentAI->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked =
                    true;
            } else if (alarmack_data->eventStateAcked ==
                CurrentAI->Event_State) {
                //! Send ack notification
            } else {
                *error_code = ERROR_CODE_INVALID_EVENT_STATE;
                return -1;
            }
            break;

        default:
            return -2;
    }
    CurrentAI->Ack_notify_data.bSendAckNotify = true;
    CurrentAI->Ack_notify_data.EventState = alarmack_data->eventStateAcked;

    return 1;
}
//------------------------------------------------------------------------------
int Analog_Input_Alarm_Summary(unsigned index,
                                  BACNET_GET_ALARM_SUMMARY_DATA *getalarm_data){
    //! Check index
    ANALOG_INPUT_DESCR *CurrentAI = NULL;
    CurrentAI = (struct analog_input_descr*)Keylist_Data(Object_List, index);
    if (CurrentAI!=NULL) {
        //! Event_State is not equal to NORMAL  and
        //! Notify_Type property value is ALARM
        if ((CurrentAI->Event_State != EVENT_STATE_NORMAL) &&
            (CurrentAI->Notify_Type == NOTIFY_ALARM)) {
            //! Object Identifier
            getalarm_data->objectIdentifier.type = OBJECT_ANALOG_INPUT;
            getalarm_data->objectIdentifier.instance =
                Analog_Input_Index_To_Instance(index);
            //! Alarm State
            getalarm_data->alarmState = CurrentAI->Event_State;
            //! Acknowledged Transitions
            bitstring_init(&getalarm_data->acknowledgedTransitions);
            bitstring_set_bit(&getalarm_data->acknowledgedTransitions,
                TRANSITION_TO_OFFNORMAL,
                CurrentAI->Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked);
            bitstring_set_bit(&getalarm_data->acknowledgedTransitions,
                TRANSITION_TO_FAULT,
                CurrentAI->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked);
            bitstring_set_bit(&getalarm_data->acknowledgedTransitions,
                TRANSITION_TO_NORMAL,
                CurrentAI->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked);

            //! Active alarm
            return 1;
        }
        else
            //! No active alarm at this index
            return 0;
    }
    else
    //! End of list
        return -1;
}
//! defined(INTRINSIC_REPORTING)
#endif
//------------------------------------------------------------------------------
bool Analog_Input_Create(uint32_t object_instance){
    bool status = false; /* return value */
    struct analog_input_descr* pObject = NULL;
    int index = 0;

    pObject = (struct analog_input_descr*)Keylist_Data(Object_List, object_instance);
    if(!pObject){
        pObject = (struct analog_input_descr*)calloc(1, sizeof(struct analog_input_descr));
        if(pObject){
            pObject->Present_Value = 12.0f;
            pObject->Out_Of_Service = false;
            pObject->Units = UNITS_NO_UNITS;
            pObject->Reliability = RELIABILITY_NO_FAULT_DETECTED;
            pObject->Prior_Value = 0.0f;
            pObject->COV_Increment = 0.0f;
            pObject->Changed = false;
            pObject->Object_Name = NULL;
            pObject->rxTime = NULL;
            pObject->rxDate = NULL;
            pObject->utcOffset = 0;
            //add to list
            index = Keylist_Data_Add(Object_List, object_instance, pObject);
            if(index>=0){
                status = true;
                Device_Inc_Database_Revision();
            }
        }
    }
    return status;
}
//------------------------------------------------------------------------------
bool Analog_Input_Delete(uint32_t object_instance){
    bool status = false;
    struct analog_input_descr *pObject = NULL;

    if(Analog_Input_Valid_Instance(object_instance)){
        unsigned int index = Analog_Input_Instance_To_Index(object_instance);
        pObject = (struct analog_input_descr*)Keylist_Data_Delete(Object_List, index);
        if(pObject){
            if(pObject->Object_Name)
                free((void*)pObject->Object_Name);
            if(pObject->rxTime)
                free((void*)pObject->rxTime);
            if(pObject->rxDate)
                free((void*)pObject->rxDate);
            free(pObject);
            status = true;
            Device_Inc_Database_Revision();
        }
    }
    return status;
}
//------------------------------------------------------------------------------
