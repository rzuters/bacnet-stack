/**************************************************************************
 *
 * Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
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

/* Multi-state Input Objects */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacenum.h"
#include "bacnet/bacapp.h"
#include "bacnet/config.h" /* the custom stuff */
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/ms-input.h"
#include "bacnet/basic/services.h"

/* number of demo objects */
#ifndef MAX_MULTISTATE_INPUTS
#define MAX_MULTISTATE_INPUTS 4
#endif

/* how many states? 1 to 254 states - 0 is not allowed. */
#ifndef MULTISTATE_NUMBER_OF_STATES
#define MULTISTATE_NUMBER_OF_STATES (254)
#endif

/* Here is our Present Value */
static uint8_t Present_Value[MAX_MULTISTATE_INPUTS];
/* Writable out-of-service allows others to manipulate our Present Value */
static bool Out_Of_Service[MAX_MULTISTATE_INPUTS];
static char Object_Name[MAX_MULTISTATE_INPUTS][64];
static char Object_Description[MAX_MULTISTATE_INPUTS][64];
static char State_Text[MAX_MULTISTATE_INPUTS][MULTISTATE_NUMBER_OF_STATES][64];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME, PROP_OBJECT_TYPE, PROP_PRESENT_VALUE, PROP_STATUS_FLAGS,
    PROP_EVENT_STATE, PROP_OUT_OF_SERVICE, PROP_NUMBER_OF_STATES, -1 };

static const int Properties_Optional[] = { PROP_DESCRIPTION, PROP_STATE_TEXT,
    -1 };

static const int Properties_Proprietary[] = { -1 };

void Multistate_Input_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    if (pRequired) {
        *pRequired = Properties_Required;
    }
    if (pOptional) {
        *pOptional = Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Properties_Proprietary;
    }

    return;
}

void Multistate_Input_Init(void)
{
    unsigned i;

    /* initialize all the analog output priority arrays to NULL */
    for (i = 0; i < MAX_MULTISTATE_INPUTS; i++) {
        Present_Value[i] = 1;
        sprintf(&Object_Name[i][0], "MULTISTATE INPUT %u", i);
        sprintf(&Object_Description[i][0], "MULTISTATE INPUT %u", i);
    }

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Multistate_Input_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = MAX_MULTISTATE_INPUTS;

    if (object_instance < MAX_MULTISTATE_INPUTS) {
        index = object_instance;
    }

    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Multistate_Input_Index_To_Instance(unsigned index)
{
    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Multistate_Input_Count(void)
{
    return MAX_MULTISTATE_INPUTS;
}

bool Multistate_Input_Valid_Instance(uint32_t object_instance)
{
    unsigned index = 0; /* offset from instance lookup */

    index = Multistate_Input_Instance_To_Index(object_instance);
    if (index < MAX_MULTISTATE_INPUTS) {
        return true;
    }

    return false;
}

static uint32_t Multistate_Input_Max_States(uint32_t instance)
{
    return MULTISTATE_NUMBER_OF_STATES;
}

uint32_t Multistate_Input_Present_Value(uint32_t object_instance)
{
    uint32_t value = 1;
    unsigned index = 0; /* offset from instance lookup */

    index = Multistate_Input_Instance_To_Index(object_instance);
    if (index < MAX_MULTISTATE_INPUTS) {
        value = Present_Value[index];
    }

    return value;
}

bool Multistate_Input_Present_Value_Set(
    uint32_t object_instance, uint32_t value)
{
    bool status = false;
    unsigned index = 0; /* offset from instance lookup */

    index = Multistate_Input_Instance_To_Index(object_instance);
    if (index < MAX_MULTISTATE_INPUTS) {
        if ((value > 0) && (value <= MULTISTATE_NUMBER_OF_STATES)) {
            Present_Value[index] = (uint8_t)value;
            status = true;
        }
    }

    return status;
}

bool Multistate_Input_Out_Of_Service(uint32_t object_instance)
{
    bool value = false;
    unsigned index = 0;

    index = Multistate_Input_Instance_To_Index(object_instance);
    if (index < MAX_MULTISTATE_INPUTS) {
        value = Out_Of_Service[index];
    }

    return value;
}

void Multistate_Input_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    unsigned index = 0;

    index = Multistate_Input_Instance_To_Index(object_instance);
    if (index < MAX_MULTISTATE_INPUTS) {
        Out_Of_Service[index] = value;
    }

    return;
}

char *Multistate_Input_Description(uint32_t object_instance)
{
    unsigned index = 0; /* offset from instance lookup */
    char *pName = NULL; /* return value */

    index = Multistate_Input_Instance_To_Index(object_instance);
    if (index < MAX_MULTISTATE_INPUTS) {
        pName = Object_Description[index];
    }

    return pName;
}

bool Multistate_Input_Description_Set(uint32_t object_instance, char *new_name)
{
    unsigned index = 0; /* offset from instance lookup */
    size_t i = 0; /* loop counter */
    bool status = false; /* return value */

    index = Multistate_Input_Instance_To_Index(object_instance);
    if (index < MAX_MULTISTATE_INPUTS) {
        status = true;
        if (new_name) {
            for (i = 0; i < sizeof(Object_Description[index]); i++) {
                Object_Description[index][i] = new_name[i];
                if (new_name[i] == 0) {
                    break;
                }
            }
        } else {
            for (i = 0; i < sizeof(Object_Description[index]); i++) {
                Object_Description[index][i] = 0;
            }
        }
    }

    return status;
}

static bool Multistate_Input_Description_Write(uint32_t object_instance,
    BACNET_CHARACTER_STRING *char_string,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    unsigned index = 0; /* offset from instance lookup */
    size_t length = 0;
    uint8_t encoding = 0;
    bool status = false; /* return value */

    index = Multistate_Input_Instance_To_Index(object_instance);
    if (index < MAX_MULTISTATE_INPUTS) {
        length = characterstring_length(char_string);
        if (length <= sizeof(Object_Description[index])) {
            encoding = characterstring_encoding(char_string);
            if (encoding == CHARACTER_UTF8) {
                status = characterstring_ansi_copy(Object_Description[index],
                    sizeof(Object_Description[index]), char_string);
                if (!status) {
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_CHARACTER_SET_NOT_SUPPORTED;
            }
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
        }
    }

    return status;
}

bool Multistate_Input_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    unsigned index = 0; /* offset from instance lookup */
    bool status = false;

    index = Multistate_Input_Instance_To_Index(object_instance);
    if (index < MAX_MULTISTATE_INPUTS) {
        status = characterstring_init_ansi(object_name, Object_Name[index]);
    }

    return status;
}

/* note: the object name must be unique within this device */
bool Multistate_Input_Name_Set(uint32_t object_instance, char *new_name)
{
    unsigned index = 0; /* offset from instance lookup */
    size_t i = 0; /* loop counter */
    bool status = false; /* return value */

    index = Multistate_Input_Instance_To_Index(object_instance);
    if (index < MAX_MULTISTATE_INPUTS) {
        status = true;
        /* FIXME: check to see if there is a matching name */
        if (new_name) {
            for (i = 0; i < sizeof(Object_Name[index]); i++) {
                Object_Name[index][i] = new_name[i];
                if (new_name[i] == 0) {
                    break;
                }
            }
        } else {
            for (i = 0; i < sizeof(Object_Name[index]); i++) {
                Object_Name[index][i] = 0;
            }
        }
    }

    return status;
}

static bool Multistate_Input_Object_Name_Write(uint32_t object_instance,
    BACNET_CHARACTER_STRING *char_string,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    unsigned index = 0; /* offset from instance lookup */
    size_t length = 0;
    uint8_t encoding = 0;
    bool status = false; /* return value */

    index = Multistate_Input_Instance_To_Index(object_instance);
    if (index < MAX_MULTISTATE_INPUTS) {
        length = characterstring_length(char_string);
        if (length <= sizeof(Object_Name[index])) {
            encoding = characterstring_encoding(char_string);
            if (encoding == CHARACTER_UTF8) {
                status = characterstring_ansi_copy(Object_Name[index],
                    sizeof(Object_Name[index]), char_string);
                if (!status) {
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_CHARACTER_SET_NOT_SUPPORTED;
            }
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
        }
    }

    return status;
}

char *Multistate_Input_State_Text(
    uint32_t object_instance, uint32_t state_index)
{
    unsigned index = 0; /* offset from instance lookup */
    char *pName = NULL; /* return value */

    index = Multistate_Input_Instance_To_Index(object_instance);
    if ((index < MAX_MULTISTATE_INPUTS) && (state_index > 0) &&
        (state_index <= MULTISTATE_NUMBER_OF_STATES)) {
        state_index--;
        pName = State_Text[index][state_index];
    }

    return pName;
}

/* note: the object name must be unique within this device */
bool Multistate_Input_State_Text_Set(
    uint32_t object_instance, uint32_t state_index, char *new_name)
{
    unsigned index = 0; /* offset from instance lookup */
    size_t i = 0; /* loop counter */
    bool status = false; /* return value */

    index = Multistate_Input_Instance_To_Index(object_instance);
    if ((index < MAX_MULTISTATE_INPUTS) && (state_index > 0) &&
        (state_index <= MULTISTATE_NUMBER_OF_STATES)) {
        state_index--;
        status = true;
        if (new_name) {
            for (i = 0; i < sizeof(State_Text[index][state_index]); i++) {
                State_Text[index][state_index][i] = new_name[i];
                if (new_name[i] == 0) {
                    break;
                }
            }
        } else {
            for (i = 0; i < sizeof(State_Text[index][state_index]); i++) {
                State_Text[index][state_index][i] = 0;
            }
        }
    }

    return status;
    ;
}

static bool Multistate_Input_State_Text_Write(uint32_t object_instance,
    uint32_t state_index,
    BACNET_CHARACTER_STRING *char_string,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    unsigned index = 0; /* offset from instance lookup */
    size_t length = 0;
    uint8_t encoding = 0;
    bool status = false; /* return value */

    index = Multistate_Input_Instance_To_Index(object_instance);
    if ((index < MAX_MULTISTATE_INPUTS) && (state_index > 0) &&
        (state_index <= Multistate_Input_Max_States(object_instance))) {
        state_index--;
        length = characterstring_length(char_string);
        if (length <= sizeof(State_Text[index][state_index])) {
            encoding = characterstring_encoding(char_string);
            if (encoding == CHARACTER_UTF8) {
                status =
                    characterstring_ansi_copy(State_Text[index][state_index],
                        sizeof(State_Text[index][state_index]), char_string);
                if (!status) {
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_CHARACTER_SET_NOT_SUPPORTED;
            }
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
        }
    } else {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
    }

    return status;
}

/* return apdu len, or BACNET_STATUS_ERROR on error */
int Multistate_Input_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int len = 0;
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    uint32_t present_value = 0;
    unsigned i = 0;
    uint32_t max_states = 0;
    bool state = false;
    uint8_t *apdu = NULL;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_MULTI_STATE_INPUT, rpdata->object_instance);
            break;
            /* note: Name and Description don't have to be the same.
               You could make Description writable and different */
        case PROP_OBJECT_NAME:
            Multistate_Input_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(&char_string,
                Multistate_Input_Description(rpdata->object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(
                &apdu[0], OBJECT_MULTI_STATE_INPUT);
            break;
        case PROP_PRESENT_VALUE:
            present_value =
                Multistate_Input_Present_Value(rpdata->object_instance);
            apdu_len = encode_application_unsigned(&apdu[0], present_value);
            break;
        case PROP_STATUS_FLAGS:
            /* note: see the details in the standard on how to use these */
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            if (Multistate_Input_Out_Of_Service(rpdata->object_instance)) {
                bitstring_set_bit(
                    &bit_string, STATUS_FLAG_OUT_OF_SERVICE, true);
            } else {
                bitstring_set_bit(
                    &bit_string, STATUS_FLAG_OUT_OF_SERVICE, false);
            }
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            /* note: see the details in the standard on how to use this */
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            state = Multistate_Input_Out_Of_Service(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_NUMBER_OF_STATES:
            apdu_len = encode_application_unsigned(&apdu[apdu_len],
                Multistate_Input_Max_States(rpdata->object_instance));
            break;
        case PROP_STATE_TEXT:
            if (rpdata->array_index == 0) {
                /* Array element zero is the number of elements in the array */
                apdu_len = encode_application_unsigned(&apdu[0],
                    Multistate_Input_Max_States(rpdata->object_instance));
            } else if (rpdata->array_index == BACNET_ARRAY_ALL) {
                /* if no index was specified, then try to encode the entire list
                 */
                /* into one packet. */
                max_states =
                    Multistate_Input_Max_States(rpdata->object_instance);
                for (i = 1; i <= max_states; i++) {
                    characterstring_init_ansi(&char_string,
                        Multistate_Input_State_Text(
                            rpdata->object_instance, i));
                    /* FIXME: this might go beyond MAX_APDU length! */
                    len = encode_application_character_string(
                        &apdu[apdu_len], &char_string);
                    /* add it if we have room */
                    if ((apdu_len + len) < MAX_APDU) {
                        apdu_len += len;
                    } else {
                        rpdata->error_code =
                            ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
                        apdu_len = BACNET_STATUS_ABORT;
                        break;
                    }
                }
            } else {
                max_states =
                    Multistate_Input_Max_States(rpdata->object_instance);
                if (rpdata->array_index <= max_states) {
                    characterstring_init_ansi(&char_string,
                        Multistate_Input_State_Text(
                            rpdata->object_instance, rpdata->array_index));
                    apdu_len = encode_application_character_string(
                        &apdu[0], &char_string);
                } else {
                    rpdata->error_class = ERROR_CLASS_PROPERTY;
                    rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                    apdu_len = BACNET_STATUS_ERROR;
                }
            }
            break;
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (rpdata->object_property != PROP_STATE_TEXT) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/* returns true if successful */
bool Multistate_Input_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    int len = 0;
    int element_len = 0;
    BACNET_APPLICATION_DATA_VALUE value;
    uint32_t max_states = 0;
    uint32_t array_index = 0;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    uint32_t object_instance = 0;

    /* decode the first chunk of the request */
    len = bacapp_decode_application_data(
        wp_data->application_data, wp_data->application_data_len, &value);
    /* len < application_data_len: extra data for arrays only */
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    if ((wp_data->object_property != PROP_STATE_TEXT) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        /*  only array properties can have array options */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_OBJECT_NAME:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_CHARACTER_STRING);
            if (status) {
                /* All the object names in a device must be unique */
                if (Device_Valid_Object_Name(&value.type.Character_String,
                        &object_type, &object_instance)) {
                    if ((object_type == wp_data->object_type) &&
                        (object_instance == wp_data->object_instance)) {
                        /* writing same name to same object */
                        status = true;
                    } else {
                        status = false;
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_DUPLICATE_NAME;
                    }
                } else {
                    status = Multistate_Input_Object_Name_Write(
                        wp_data->object_instance, &value.type.Character_String,
                        &wp_data->error_class, &wp_data->error_code);
                }
            }
            break;
        case PROP_DESCRIPTION:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_CHARACTER_STRING);
            if (status) {
                status = Multistate_Input_Description_Write(
                    wp_data->object_instance, &value.type.Character_String,
                    &wp_data->error_class, &wp_data->error_code);
            }
            break;
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                status = Multistate_Input_Present_Value_Set(
                    wp_data->object_instance, value.type.Unsigned_Int);
                if (!status) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Multistate_Input_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
        case PROP_STATE_TEXT:
            if (wp_data->array_index == 0) {
                /* Array element zero is the number of
                   elements in the array.  We have a fixed
                   size array, so we are read-only. */
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            } else if (wp_data->array_index == BACNET_ARRAY_ALL) {
                max_states =
                    Multistate_Input_Max_States(wp_data->object_instance);
                array_index = 1;
                element_len = len;
                do {
                    status = write_property_type_valid(wp_data, &value,
                        BACNET_APPLICATION_TAG_CHARACTER_STRING);
                    if (!status) {
                        break;
                    }
                    if (element_len) {
                        status = Multistate_Input_State_Text_Write(
                            wp_data->object_instance, array_index,
                            &value.type.Character_String, &wp_data->error_class,
                            &wp_data->error_code);
                    }
                    max_states--;
                    array_index++;
                    if (max_states) {
                        element_len = bacapp_decode_application_data(
                            &wp_data->application_data[len],
                            wp_data->application_data_len - len, &value);
                        if (element_len < 0) {
                            wp_data->error_class = ERROR_CLASS_PROPERTY;
                            wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                            break;
                        }
                        len += element_len;
                    }
                } while (max_states);
            } else {
                max_states =
                    Multistate_Input_Max_States(wp_data->object_instance);
                if (wp_data->array_index <= max_states) {
                    status = write_property_type_valid(wp_data, &value,
                        BACNET_APPLICATION_TAG_CHARACTER_STRING);
                    if (status) {
                        status = Multistate_Input_State_Text_Write(
                            wp_data->object_instance, wp_data->array_index,
                            &value.type.Character_String, &wp_data->error_class,
                            &wp_data->error_code);
                    }
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_TYPE:
        case PROP_STATUS_FLAGS:
        case PROP_EVENT_STATE:
        case PROP_NUMBER_OF_STATES:
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
