/*
 Copyright (C) AC SOFTWARE SP. Z O.O.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <eagle_soc.h>
#include <ets_sys.h>
#include <os_type.h>
#include <osapi.h>
#include "supla-dev/log.h"

#include "supla_esp_devconn.h"
#include "supla_esp_electricity_meter.h"

#ifdef ELECTRICITY_METER

ETSTimer supla_em_timer1;
char supla_em_send_base_enabled = 1;

void ICACHE_FLASH_ATTR supla_esp_em_extendedvalue_to_value(
    TElectricityMeter_ExtendedValue *ev, char *value);

void ICACHE_FLASH_ATTR supla_esp_em_on_timer(void *ptr) {
  if (supla_esp_devconn_is_registered() != 1) {
    return;
  }

  unsigned char channel_number = 0;
  char value[SUPLA_CHANNELVALUE_SIZE];
  TElectricityMeter_ExtendedValue ev;
  memset(&ev, 0, sizeof(TElectricityMeter_ExtendedValue));

  while (channel_number < 100 &&
         supla_esp_board_get_measurements(channel_number, &ev) == 1) {
    supla_esp_em_extendedvalue_to_value(&ev, value);
    if (supla_em_send_base_enabled == 1) {
      supla_esp_channel_value__changed(channel_number, value);
    }
    supla_esp_channel_em_value_changed(channel_number, &ev);
    memset(&ev, 0, sizeof(TElectricityMeter_ExtendedValue));
    channel_number++;
  }
}

void ICACHE_FLASH_ATTR supla_esp_em_init(void) {}

void ICACHE_FLASH_ATTR supla_esp_em_start(void) {
  os_timer_disarm(&supla_em_timer1);
  os_timer_setfn(&supla_em_timer1, (os_timer_func_t *)supla_esp_em_on_timer,
                 NULL);
  os_timer_arm(&supla_em_timer1, 5000, 1);
}

void ICACHE_FLASH_ATTR supla_esp_em_device_registered(void) {
  supla_esp_em_on_timer(NULL);
}

void ICACHE_FLASH_ATTR supla_esp_em_extendedvalue_to_value(
    TElectricityMeter_ExtendedValue *ev, char *value) {
  memset(value, 0, SUPLA_CHANNELVALUE_SIZE);

  if (sizeof(TElectricityMeter_Value) > SUPLA_CHANNELVALUE_SIZE) {
    return;
  }

  TElectricityMeter_Measurement *m = NULL;
  TElectricityMeter_Value v;
  memset(&v, 0, sizeof(TElectricityMeter_Value));

  unsigned _supla_int64_t fae_sum = ev->total_forward_active_energy[0] +
                                    ev->total_forward_active_energy[1] +
                                    ev->total_forward_active_energy[2];

  v.total_forward_active_energy = fae_sum / 1000;

  if (ev->m_count && ev->measured_values & EM_VAR_VOLTAGE) {
    m = &ev->m[ev->m_count - 1];

    if (m->voltage[0] > 0) {
      v.flags |= EM_VALUE_FLAG_PHASE1_ON;
    }

    if (m->voltage[1] > 0) {
      v.flags |= EM_VALUE_FLAG_PHASE2_ON;
    }

    if (m->voltage[2] > 0) {
      v.flags |= EM_VALUE_FLAG_PHASE3_ON;
    }
  }

  memcpy(value, &v, sizeof(TElectricityMeter_Value));
}

void ICACHE_FLASH_ATTR supla_esp_em_get_value(
    unsigned char channel_number, char value[SUPLA_CHANNELVALUE_SIZE]) {
  TElectricityMeter_ExtendedValue ev;
  memset(&ev, 0, sizeof(TElectricityMeter_ExtendedValue));
  if (supla_esp_board_get_measurements(channel_number, &ev) == 1) {
    supla_esp_em_extendedvalue_to_value(&ev, value);
  }
}

void ICACHE_FLASH_ATTR supla_esp_em_send_base_value_enabled(char enabled) {
  supla_em_send_base_enabled = enabled;
}

void ICACHE_FLASH_ATTR supla_esp_em_set_measurement_frequency(int freq) {
  os_timer_disarm(&supla_em_timer1);
  if (freq >= 1000) {
    os_timer_arm(&supla_em_timer1, freq, 1);
  }
}

#endif /*ELECTRICITY_METER*/
