 /* Copyright 2022 Bogdan Pilyugin
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *   File name: SNTP.c
 *     Project: ChargePointMainboard
 *  Created on: 2022-07-21
 *      Author: Bogdan Pilyugin
 * Description:	
 */

#include "esp_sntp.h"
#include "NetTransport.h"
#define YEAR_BASE (1900) //tm structure base year

static void initialize_sntp(void);


static void obtain_time(void *pvParameter)
{
  initialize_sntp();

  // wait for time to be set
  int retry = 0;
  const int retry_count = 10;
  while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

static void time_sync_notification_cb(struct timeval *tv)
{

}

static void initialize_sntp(void)
{
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");
  sntp_set_sync_interval(6*3600*1000);
  sntp_set_time_sync_notification_cb(time_sync_notification_cb);
  sntp_init();
}

void StartTimeGet(void)
{
    xTaskCreate(obtain_time, "ObtainTimeTask", 1024 * 4, (void*) 0, 3, NULL);
}

void GetRFC3339Time(char *t)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    sprintf(t, "%04d-%02d-%02dT%02d:%02d:%02d+00:00",
                (timeinfo.tm_year) + YEAR_BASE,
                (timeinfo.tm_mon) + 1,
                timeinfo.tm_mday,
                timeinfo.tm_hour,
                timeinfo.tm_min,
                timeinfo.tm_sec);
}


