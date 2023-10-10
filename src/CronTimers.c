/* Copyright 2023 Bogdan Pilyugin
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
 *   File name: CronTimers.c
 *     Project: webguiapp_ref_implement
 *  Created on: 2023-04-15
 *      Author: bogdan
 * Description:	
 */

//{
// "data":{
// "msgid":123456789,
// "time":"2023-06-03T12:25:24+00:00",
// "msgtype":1,"payloadtype":1, "payload":{"applytype":1,
// "variables":{
// "cronrecs":[{ "num": 1, "del": 0, "enab": 1, "prev": 0, "name": "Timer Name", "obj": 0, "act": 0,
//              "cron": "*/3 * * * * *",
//              "exec": "OUTPUTS,TEST,ARGUMENTS"
//                }]
// }}},"signature":"6a11b872e8f766673eb82e127b6918a0dc96a42c5c9d184604f9787f3d27bcef"}
#include <CronTimers.h>
#include "esp_log.h"
#include "webguiapp.h"

#define TAG "CRON_TIMER"

extern obj_struct_t app_com_obj_arr[];
extern obj_struct_t com_obj_arr[];

const char *cron_actions[] = { "ON", "REBOOT", "TOGGLE", "OFF", "VERYLONG_OPERATION" };
const char *cron_objects[] = {
        "RELAY1",
        "RELAY2",
        "RELAY3",
        "RELAY4",
        "RELAY5",
        "RELAY6",
        "RELAY7",
        "RELAY8",
        "SYSTEM" };
const char *cron_act_avail[] = {
        "[0,2,3]",
        "[0,2,3]",
        "[0,2,3]",
        "[0,2,3]",
        "[0,2,3]",
        "[0,2,3]",
        "[0,2,3]",
        "[0,2,3]",
        "[1,4]" };

char* GetCronObjectNameDef(int idx)
{
    if (idx < 0 || idx >= sizeof(cron_objects) / sizeof(char*))
        return "";
    return (char*) cron_objects[idx];
}

char* GetCronObjectName(int idx)
{
    if (idx < 0 || idx >= sizeof(cron_objects) / sizeof(char*))
        return "";
    return GetSysConf()->CronObjects[idx].objname;
}

char* GetCronActionName(int idx)
{
    if (idx < 0 || idx >= sizeof(cron_actions) / sizeof(char*))
        return "";
    return (char*) cron_actions[idx];
}

char* GetCronActAvail(int idx)
{
    if (idx < 0 || idx >= sizeof(cron_act_avail) / sizeof(char*))
        return "[]";
    return (char*) cron_act_avail[idx];
}

static cron_job *JobsList[CRON_TIMERS_NUMBER];
static char cron_express_error[CRON_EXPRESS_MAX_LENGTH];

char* GetCronError()
{
    return cron_express_error;
}

/**
 * \brief Handle all actions under all objects
 * \param obj  Index of the object
 * \param act  Index of the action
 */
void custom_cron_execute(int obj, int act)
{

}

void custom_cron_job_callback(cron_job *job)
{
    ExecCommand(((cron_timer_t*) job->data)->exec);
}

esp_err_t InitCronSheduler()
{
    esp_err_t res = ESP_OK;
    return res;
}

const char* check_expr(const char *expr)
{
    const char *err = NULL;
    cron_expr test;
    memset(&test, 0, sizeof(test));
    cron_parse_expr(expr, &test, &err);
    return err;
}

static void ExecuteLastAction()
{
    int obj;
    for (obj = 0; obj < CONFIG_WEBGUIAPP_MAX_OBJECTS_NUM; obj++)
    {
        int shdl;
        time_t now;
        time(&now);
        time_t delta = now;
        int act = -1;
        int minimal = -1;


        char *obj = GetSystemObjects()->object_name;

        for (shdl = 0; shdl < CRON_TIMERS_NUMBER; shdl++)
        {
            char *obj_in_cron = strtok(GetSysConf()->Timers[shdl].exec, ',');

            if (GetSysConf()->Timers[shdl].enab &&
                    !GetSysConf()->Timers[shdl].del &&
                    GetSysConf()->Timers[shdl].prev &&
                    !strcmp(obj, obj_in_cron))

            {
                cron_expr cron_exp = { 0 };
                cron_parse_expr(GetSysConf()->Timers[shdl].cron, &cron_exp, NULL);
                time_t prev = cron_prev(&cron_exp, now);
                if ((now - prev) < delta)
                {
                    delta = (now - prev);
                    minimal = shdl;
                }
            }
        }

        if (shdl != -1)
        {

            ExecCommand(GetSysConf()->Timers[shdl].exec);
        }
    }
}

void TimeObtainHandler(struct timeval *tm)
{
    ESP_LOGW(TAG, "Current time received with value %d", (unsigned int )tm->tv_sec);
    ReloadCronSheduler();
    ExecuteLastAction();
    LogFile("cron.log", "Cron service started");
}

void DebugTimer()
{
    ExecuteLastAction();
}

esp_err_t ReloadCronSheduler()
{
    //remove all jobs
    ESP_LOGI(TAG, "Cron stop call result %d", cron_stop());
    cron_job_clear_all();
    //check if we have jobs to run
    bool isExpressError = false;
    for (int i = 0; i < CRON_TIMERS_NUMBER; i++)
    {
        const char *err = check_expr(GetSysConf()->Timers[i].cron);
        if (err)
        {
            snprintf(cron_express_error, CRON_EXPRESS_MAX_LENGTH - 1, "In timer %d expression error:%s", i + 1, err);
            ESP_LOGE(TAG, "%s", cron_express_error);
            isExpressError = true;
            continue;
        }
        else if (!GetSysConf()->Timers[i].del && GetSysConf()->Timers[i].enab)
        {
            JobsList[i] = cron_job_create(GetSysConf()->Timers[i].cron, custom_cron_job_callback,
                                          (void*) &GetSysConf()->Timers[i]);
        }
    }
    if (!isExpressError)
        cron_express_error[0] = 0x00; //clear last cron expression parse
    int jobs_num = cron_job_node_count();
    ESP_LOGI(TAG, "In config presents %d jobs", jobs_num);

    if (jobs_num > 0)
        ESP_LOGI(TAG, "Cron start call result %d", cron_start());
    return ESP_OK;
}

