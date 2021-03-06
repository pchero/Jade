/*
 * campaign_handler.c
 *
 *  Created on: Nov 26, 2015
 *    Author: pchero
 */


#define _GNU_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <jansson.h>

#include "common.h"
#include "slog.h"
#include "utils.h"
#include "ob_campaign_handler.h"
#include "ob_dl_handler.h"
#include "ob_plan_handler.h"
#include "ob_dlma_handler.h"

#define DEF_CAMPAIGN_SCHEDULE_MODE  E_CAMP_SCHEDULE_OFF
#define DEF_CAMPAIGN_STATUS E_CAMP_STOP

static json_t* get_deleted_ob_campaign(const char* uuid);
static json_t* create_ob_campaign_default(void);
static json_t* get_campaigns_uuid_by_status_schedule(E_CAMP_STATUS_T status, E_CAMP_SCHEDULE_MODE mode);
static json_t* get_ob_campaigns_uuid_by_status(E_CAMP_STATUS_T status);

static bool is_startable_campaign_schedule(json_t* j_camp);
static bool is_startable_campaign_schedule_day(json_t* j_camp, int day);
static bool is_startable_campaign_schedule_check(json_t* j_camp, const char* cur_date, const char* cur_time, int cur_day);
static bool is_stopable_campaign_schedule_day_date_list(json_t* j_camp);
static bool is_stoppable_campaign_schedule_check(json_t* j_camp, const char* cur_date, const char* cur_time, int cur_day);

extern db_ctx_t* g_db_ob;

static json_t* create_ob_campaign_default(void)
{
  json_t* j_res;

  j_res = json_pack("{"
      "s:o, s:o, s:i, "
      "s:o, s:o, s:o, "
      "s:o, "
      "s:i, s:o, s:o, s:o, s:o, s:o, s:o, s:o, "
      "s:o"
      "}",

      "name",   json_null(),
      "detail", json_null(),
      "status", DEF_CAMPAIGN_STATUS,

      "plan", json_null(),
      "dlma", json_null(),
      "dest", json_null(),

      "next_campaign",  json_null(),

      // schedule
      "sc_mode",              DEF_CAMPAIGN_SCHEDULE_MODE,
      "sc_date_start",        json_null(),
      "sc_date_end",          json_null(),
      "sc_date_list",         json_null(),
      "sc_date_list_except",  json_null(),
      "sc_time_start",        json_null(),
      "sc_time_end",          json_null(),
      "sc_day_list",          json_null(),

      "variables",  json_object()
      );

  return j_res;
}

/**
 * Create ob campaign.
 * @param j_camp
 * @return
 */
json_t* ob_create_campaign(json_t* j_camp)
{
  int ret;
  char* uuid;
  char* tmp;
  json_t* j_tmp;

  if(j_camp == NULL) {
    slog(LOG_DEBUG, "Wrong input parameter.");
    return NULL;
  }
  slog(LOG_DEBUG, "Fired create_ob_campaign.");

  j_tmp = create_ob_campaign_default();
  json_object_update_existing(j_tmp, j_camp);

  uuid = utils_gen_uuid();
  json_object_set_new(j_tmp, "uuid", json_string(uuid));

  tmp = utils_get_utc_timestamp();
  json_object_set_new(j_tmp, "tm_create", json_string(tmp));
  sfree(tmp);

  slog(LOG_NOTICE, "Create ob_campaign. uuid[%s], name[%s]",
      json_string_value(json_object_get(j_tmp, "uuid")),
      json_string_value(json_object_get(j_tmp, "name"))
      );
  ret = db_ctx_insert(g_db_ob, "ob_campaign", j_tmp);
  json_decref(j_tmp);
  if(ret == false) {
    sfree(uuid);
    return NULL;
  }

  j_tmp = ob_get_campaign(uuid);
  if(j_tmp == NULL) {
    slog(LOG_ERR, "Could not create campaign info correctly. uuid[%s]", uuid);
    sfree(uuid);
    return NULL;
  }
  sfree(uuid);

  return j_tmp;
}

/**
 * Delete outbound campaign.
 * @param uuid
 * @return
 */
json_t* ob_delete_campaign(const char* uuid)
{
  json_t* j_tmp;
  json_t* j_res;
  char* tmp;
  char* sql;
  int ret;

  if(uuid == NULL) {
    // invalid parameter.
    slog(LOG_WARNING, "Wrong input parameter.");
    return NULL;
  }
  slog(LOG_DEBUG, "Fired delete_ob_campaign. uuid[%s]", uuid);

  j_tmp = json_object();
  tmp = utils_get_utc_timestamp();
  json_object_set_new(j_tmp, "status", json_integer(E_CAMP_STOP));
  json_object_set_new(j_tmp, "tm_delete", json_string(tmp));
  json_object_set_new(j_tmp, "in_use", json_integer(E_USE_NO));
  sfree(tmp);

  tmp = db_ctx_get_update_str(j_tmp);
  json_decref(j_tmp);
  asprintf(&sql, "update ob_campaign set %s where uuid=\"%s\";", tmp, uuid);
  sfree(tmp);

  ret = db_ctx_exec(g_db_ob, sql);
  sfree(sql);
  if(ret == false) {
    slog(LOG_WARNING, "Could not delete campaign. uuid[%s]", uuid);
    return NULL;
  }

  j_res = get_deleted_ob_campaign(uuid);
  if(j_res == NULL) {
    slog(LOG_ERR, "Could not get deleted campaign info. uuid[%s]", uuid);
    return NULL;
  }

  return j_res;
}

static json_t* get_ob_campaign_use(const char* uuid, E_USE use)
{
  int ret;
  char* sql;
  json_t* j_res;

  if(uuid == NULL) {
    slog(LOG_WARNING, "Invalid input parameters.");
    return NULL;
  }
  slog(LOG_DEBUG, "Fired get_ob_campaign_use. uuid[%s], use[%d]", uuid, use);

  asprintf(&sql, "select * from ob_campaign where uuid=\"%s\" and in_use=%d;", uuid, use);

  ret = db_ctx_query(g_db_ob, sql);
  sfree(sql);
  if(ret == false) {
    slog(LOG_ERR, "Could not get ob_campaign info. uuid[%s], use[%d]", uuid, use);
    return NULL;
  }
  j_res = db_ctx_get_record(g_db_ob);

  db_ctx_free(g_db_ob);

  return j_res;
}


/**
 * Get specified campaign
 * @return
 */
json_t* ob_get_campaign(const char* uuid)
{
  json_t* j_res;

  if(uuid == NULL) {
    return NULL;
  }
  slog(LOG_DEBUG, "Fired get_ob_campaign. uuid[%s]", uuid);

  j_res = get_ob_campaign_use(uuid, E_USE_OK);
  if(j_res == NULL) {
    slog(LOG_WARNING, "Could not get ob_campaign info.");
    return NULL;
  }

  return j_res;
}

/**
 * Get deleted campaign.
 * @return
 */
static json_t* get_deleted_ob_campaign(const char* uuid)
{
  json_t* j_res;

  if(uuid == NULL) {
    return NULL;
  }
  slog(LOG_DEBUG, "Fired get_deleted_ob_campaign info. uuid[%s]", uuid);

  j_res = get_ob_campaign_use(uuid, E_USE_NO);
  if(j_res == NULL) {
    slog(LOG_WARNING, "Could not get deleted ob_campaign info.");
    return NULL;
  }

  return j_res;
}

/**
 * Get all campaigns
 * @return
 */
json_t* ob_get_campaigns_all(void)
{
  int ret;
  char* sql;
  json_t* j_res;
  json_t* j_tmp;

  asprintf(&sql, "select * from ob_campaign where in_use=%d;", E_USE_OK);
  ret = db_ctx_query(g_db_ob, sql);
  sfree(sql);
  if(ret == false) {
    slog(LOG_WARNING, "Could not get correct ob_campaign.");
    return NULL;
  }

  j_res = json_array();
  while(1) {
    j_tmp = db_ctx_get_record(g_db_ob);
    if(j_tmp == NULL) {
      break;
    }

    json_array_append_new(j_res, j_tmp);
  }
  db_ctx_free(g_db_ob);

  return j_res;
}


/**
 * Return the json array of all campaign uuid.
 * @return
 */
json_t* ob_get_campaigns_all_uuid(void)
{
  int ret;
  char* sql;
  json_t* j_res;
  json_t* j_res_tmp;
  json_t* j_tmp;

  asprintf(&sql, "select uuid from ob_campaign where in_use=%d;", E_USE_OK);
  ret = db_ctx_query(g_db_ob, sql);
  sfree(sql);
  if(ret == false) {
    slog(LOG_WARNING, "Could not get correct ob_campaign.");
    return NULL;
  }

  j_res = json_array();
  while(1) {
    j_tmp = db_ctx_get_record(g_db_ob);
    if(j_tmp == NULL) {
      break;
    }

    j_res_tmp = json_pack("{s:s}",
        "uuid", json_string_value(json_object_get(j_tmp, "uuid"))
        );
    json_decref(j_tmp);
    if(j_res_tmp == NULL) {
      continue;
    }

    json_array_append_new(j_res, j_res_tmp);
  }
  db_ctx_free(g_db_ob);

  return j_res;
}

static json_t* get_campaigns_uuid_by_status_schedule(E_CAMP_STATUS_T status, E_CAMP_SCHEDULE_MODE mode)
{
  int ret;
  json_t* j_res;
  json_t* j_tmp;
  char* sql;
  const char* uuid;

  asprintf(&sql, "select uuid from ob_campaign where status == %d and sc_mode == %d;",
      status,
      mode
      );

  ret = db_ctx_query(g_db_ob, sql);
  sfree(sql);
  if(ret == false) {
    return NULL;
  }

  j_res = json_array();
  while(1) {

    j_tmp = db_ctx_get_record(g_db_ob);
    if(j_tmp == NULL) {
      break;
    }

    uuid = json_string_value(json_object_get(j_tmp, "uuid"));
    if(uuid == NULL) {
      json_decref(j_tmp);
      continue;
    }

    json_array_append_new(j_res, json_string(uuid));
    json_decref(j_tmp);
  }
  db_ctx_free(g_db_ob);

  return j_res;
}

/**
 * Get all start-able campaigns by schedule.
 * \return
 */
json_t* ob_get_campaigns_schedule_start(void)
{
  json_t* j_res;
  json_t* j_tmp;
  json_t* j_uuids;
  int ret;
  unsigned int idx;
  json_t* j_val;
  const char* uuid;

  // get scheduled campaigns uuid
  j_uuids = get_campaigns_uuid_by_status_schedule(E_CAMP_STOP, E_CAMP_SCHEDULE_ON);

  j_res = json_array();
  json_array_foreach(j_uuids, idx, j_val) {

    uuid = json_string_value(j_val);
    if(uuid == NULL) {
      continue;
    }

    j_tmp = ob_get_campaign(uuid);
    if(j_tmp == NULL) {
      continue;
    }

    ret = is_startable_campaign_schedule(j_tmp);
    if(ret == false) {
      json_decref(j_tmp);
      continue;
    }

    json_array_append_new(j_res, j_tmp);
  }
  json_decref(j_uuids);

  return j_res;
}

/**
 * Get all stop-able campaigns by schedule.
 * \return
 */
json_t* ob_get_campaigns_schedule_end(void)
{
  json_t* j_res;
  json_t* j_tmp;
  json_t* j_uuids;
  int ret;
  unsigned int idx;
  json_t* j_val;
  const char* uuid;

  // get scheduled campaigns uuid
  j_uuids = get_campaigns_uuid_by_status_schedule(E_CAMP_START, E_CAMP_SCHEDULE_ON);

  j_res = json_array();
  json_array_foreach(j_uuids, idx, j_val) {

    uuid = json_string_value(j_val);
    if(uuid == NULL) {
      continue;
    }

    j_tmp = ob_get_campaign(uuid);
    if(j_tmp == NULL) {
      continue;
    }

    ret = is_stopable_campaign_schedule_day_date_list(j_tmp);
    if(ret == false) {
      json_decref(j_tmp);
      continue;
    }

    json_array_append_new(j_res, j_tmp);
  }
  json_decref(j_uuids);

  return j_res;
}

/**
 * Update ob_campaign
 * @param j_camp
 * @return
 */
json_t* ob_update_campaign(const json_t* j_camp)
{
  char* tmp;
  const char* tmp_const;
  char* sql;
  json_t* j_tmp;
  char* uuid;

  if(j_camp == NULL) {
    return NULL;
  }

  j_tmp = json_deep_copy(j_camp);
  if(j_tmp == NULL) {
    return NULL;
  }

  tmp_const = json_string_value(json_object_get(j_tmp, "uuid"));
  if(tmp_const == NULL) {
    slog(LOG_WARNING, "Could not get uuid info.");
    json_decref(j_tmp);
    return NULL;
  }
  uuid = strdup(tmp_const);

  tmp = utils_get_utc_timestamp();
  json_object_set_new(j_tmp, "tm_update", json_string(tmp));
  sfree(tmp);

  tmp = db_ctx_get_update_str(j_tmp);
  json_decref(j_tmp);
  if(tmp == NULL) {
    slog(LOG_WARNING, "Could not get update str.");
    sfree(uuid);
    return NULL;
  }

  asprintf(&sql, "update ob_campaign set %s where uuid=\"%s\" and in_use=%d;", tmp, uuid, E_USE_OK);
  sfree(tmp);

  // update
  db_ctx_exec(g_db_ob, sql);
  sfree(sql);

  slog(LOG_DEBUG, "Getting updated campaign info. uuid[%s]", uuid);
  j_tmp = ob_get_campaign(uuid);
  sfree(uuid);
  if(j_tmp == NULL) {
    slog(LOG_WARNING, "Could not get updated campaign info.");
    return NULL;
  }

  return j_tmp;
}

/**
 * Update campaign status info.
 * @param uuid
 * @param status
 * @return
 */
bool ob_update_campaign_status(const char* uuid, E_CAMP_STATUS_T status)
{
  char* tmp_status;
  json_t* j_camp;
  json_t* j_tmp;

  if(uuid == NULL) {
    slog(LOG_WARNING, "Invalid input parameters.");
    return false;
  }

  if(status == E_CAMP_START)      tmp_status = "run";
  else if(status == E_CAMP_STOP)    tmp_status = "stop";
  else if(status == E_CAMP_PAUSE)   tmp_status = "pause";
  else if(status == E_CAMP_STARTING)  tmp_status = "running";
  else if(status == E_CAMP_STOPPING)  tmp_status = "stopping";
  else if(status == E_CAMP_PAUSING)   tmp_status = "pausing";
//  else if(status == E_CAMP_SCHEDULE_STOP)       tmp_status = "schedule_stop";
//  else if(status == E_CAMP_SCHEDULE_STOPPING)   tmp_status = "schedule_stopping";
  else {
    slog(LOG_WARNING, "Invalid input parameters.");
    return false;
  }
  slog(LOG_NOTICE, "update ob_campaign status. uuid[%s], status[%d], status_string[%s]", uuid, status, tmp_status);

  // get campaign
  j_camp = ob_get_campaign(uuid);
  if(j_camp == NULL) {
    slog(LOG_WARNING, "Could not find campaign. camp_uuid[%s]", uuid);
    return false;
  }

  // set status
  json_object_set_new(j_camp, "status", json_integer(status));

  // update
  j_tmp = ob_update_campaign(j_camp);
  json_decref(j_camp);
  if(j_tmp == NULL) {
    slog(LOG_ERR, "Could not update campaign status. camp_uuid[%s], status[%d]", uuid, status);
    return false;
  }
  json_decref(j_tmp);

  return true;
}

static json_t* get_ob_campaigns_uuid_by_status(E_CAMP_STATUS_T status)
{
  int ret;
  json_t* j_res;
  json_t* j_tmp;
  char* sql;
  const char* uuid;

  asprintf(&sql, "select uuid from ob_campaign where status = %d and in_use=%d;",
      status, E_USE_OK
      );

  ret = db_ctx_query(g_db_ob, sql);
  sfree(sql);
  if(ret == false) {
    slog(LOG_WARNING, "Could not get ob_campaign info.");
    return NULL;
  }

  j_res = json_array();
  while(1) {
    j_tmp = db_ctx_get_record(g_db_ob);
    if(j_tmp == NULL) {
      break;
    }

    uuid = json_string_value(json_object_get(j_tmp, "uuid"));
    if(uuid == NULL) {
      json_decref(j_tmp);
      continue;
    }

    json_array_append_new(j_res, json_string(uuid));
    json_decref(j_tmp);
  }
  db_ctx_free(g_db_ob);

  return j_res;
}

/**
 * Get campaign for dialing.
 * @return
 */
json_t* ob_get_campaigns_by_status(E_CAMP_STATUS_T status)
{
  json_t* j_uuids;
  json_t* j_res;
  json_t* j_tmp;
  json_t* j_val;
  const char* uuid;
  unsigned int idx;

  j_uuids = get_ob_campaigns_uuid_by_status(status);

  j_res = json_array();
  json_array_foreach(j_uuids, idx, j_val) {
    uuid = json_string_value(j_val);
    if(uuid == NULL) {
      continue;
    }

    j_tmp = ob_get_campaign(uuid);
    json_array_append_new(j_res, j_tmp);
  }
  json_decref(j_uuids);

  return j_res;
}

/**
 * Get campaign for dialing.
 * @return
 */
json_t* ob_get_campaign_for_dialing(void)
{
  int ret;
  const char* uuid;
  json_t* j_res;
  json_t* j_tmp;
  char* sql;

  // get "start" status campaign only.
  asprintf(&sql, "select uuid from ob_campaign where status = %d and in_use = %d order by %s limit 1;",
      E_CAMP_START,
      E_USE_OK,
      "random()"
      );

  ret = db_ctx_query(g_db_ob, sql);
  sfree(sql);
  if(ret == false) {
    slog(LOG_WARNING, "Could not get ob_campaign info.");
    return NULL;
  }

  j_tmp = db_ctx_get_record(g_db_ob);
  db_ctx_free(g_db_ob);
  if(j_tmp == NULL) {
    return NULL;
  }

  uuid = json_string_value(json_object_get(j_tmp, "uuid"));
  j_res = ob_get_campaign(uuid);
  json_decref(j_tmp);

  return j_res;
}

/**
 *
 * \param uuid
 * \return
 */
json_t* ob_get_campaign_stat(const char* uuid)
{
  json_t* j_camp;
  json_t* j_plan;
  json_t* j_dlma;
  json_t* j_res;

  if(uuid == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return NULL;
  }

  // get campaign
  j_camp = ob_get_campaign(uuid);
  if(j_camp == NULL) {
    slog(LOG_DEBUG, "Could not get ob_campaign info. camp_uuid[%s]", uuid);
    return NULL;
  }

  // get plan
  j_plan = ob_get_plan(json_string_value(json_object_get(j_camp, "plan")));
  j_dlma = ob_get_dlma(json_string_value(json_object_get(j_camp, "dlma")));
  if((j_plan == NULL) || (j_dlma == NULL)) {
    slog(LOG_DEBUG, "Could not basic info. camp_uuid[%s], plan_uuid[%s], dlma_uuid[%s]",
        uuid,
        json_string_value(json_object_get(j_camp, "plan")),
        json_string_value(json_object_get(j_camp, "dlma"))
        );
    json_decref(j_camp);
    return NULL;
  }

  // create
  j_res = json_pack("{"
      "s:s, "
      "s:i, s:i, s:i, s:i, s:i"
      "}",

      "uuid",                  json_string_value(json_object_get(j_camp, "uuid"))? : "",

      "dial_total_count",     ob_get_dl_list_cnt_total(j_dlma),
      "dial_finished_count",  ob_get_dl_list_cnt_finshed(j_dlma, j_plan),
      "dial_available_count", ob_get_dl_list_cnt_available(j_dlma, j_plan),
      "dial_dialing_count",   ob_get_dl_list_cnt_dialing(j_dlma),
      "dial_called_count",    ob_get_dl_list_cnt_tried(j_dlma)
      );

  json_decref(j_camp);
  json_decref(j_plan);
  json_decref(j_dlma);

  return j_res;
}

json_t* ob_get_campaigns_stat_all(void)
{
  json_t* j_stats;
  json_t* j_stat;
  json_t* j_camps;
  json_t* j_camp;
  int i;
  int size;

  j_camps = ob_get_campaigns_all();
  size = json_array_size(j_camps);
  j_stats = json_array();
  for(i = 0; i < size; i++) {
    j_camp = json_array_get(j_camps, i);
    if(j_camp == NULL) {
      continue;
    }

    j_stat = ob_get_campaign_stat(json_string_value(json_object_get(j_camp, "uuid")));
    if(j_stat == NULL) {
      continue;
    }

    json_array_append_new(j_stats, j_stat);
  }

  json_decref(j_camps);

  return j_stats;
}


/**
 * return the possibility of status change to start
 */
bool ob_is_startable_campgain(json_t* j_camp)
{
  if(j_camp == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return false;
  }

  return true;
}

/**
 * return the possibility of status change to stop
 */
bool ob_is_stoppable_campgain(json_t* j_camp)
{
  int ret;
  char* sql;
  json_t* j_tmp;

  if(j_camp == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return false;
  }
  slog(LOG_DEBUG, "Fired is_stoppable_campgain. uuid[%s], name[%s]",
      json_string_value(json_object_get(j_camp, "uuid"))? : "",
      json_string_value(json_object_get(j_camp, "name"))? : ""
      );

  asprintf(&sql, "select count(*) from ob_dialing where uuid_camp=\"%s\";",
      json_string_value(json_object_get(j_camp, "uuid"))? : ""
      );

  ret = db_ctx_query(g_db_ob, sql);
  sfree(sql);
  if(ret == false) {
    slog(LOG_INFO, "Could not get correct stoppable campaign info.");
    return false;
  }

  j_tmp = db_ctx_get_record(g_db_ob);
  db_ctx_free(g_db_ob);
  if(j_tmp == NULL) {
    slog(LOG_INFO, "Could not get correct stoppable campaign info.");
    return false;
  }

  ret = json_integer_value(json_object_get(j_tmp, "count(*)"));
  json_decref(j_tmp);
  if(ret > 0) {
    return false;
  }

  return true;
}

static bool is_startable_campaign_schedule(json_t* j_camp)
{
  char* cur_date;
  char* cur_time;
  int cur_day;
  int ret;

  if(j_camp == NULL) {
    return false;
  }

  cur_date = utils_get_utc_timestamp_date();
  cur_time = utils_get_utc_timestamp_time();
  cur_day = utils_get_utc_timestamp_day();

  // check startable date
  ret = is_startable_campaign_schedule_check(j_camp, cur_date, cur_time, cur_day);
  sfree(cur_date);
  sfree(cur_time);
  if(ret == true) {
    return true;
  }

  return false;
}

static bool is_startable_campaign_schedule_check(
    json_t* j_camp,
    const char* cur_date,
    const char* cur_time,
    int cur_day
    )
{
  const char* date_list;
  const char* date_except;
  char* tmp;
  const char* tmp_const;
  int ret;

  if((j_camp == NULL) || (cur_date == NULL) || (cur_time == NULL)) {
    return false;
  }

  // check except date. If there's except date, return false
  date_except = json_string_value(json_object_get(j_camp, "sc_date_list_except"));
  if(date_except != NULL) {
    tmp = strstr(date_except, cur_date);
    if(tmp != NULL) {
      return false;
    }
  }

  // check date_list. If there's current date, return true
  date_list = json_string_value(json_object_get(j_camp, "sc_date_list"));
  if(date_list != NULL) {
    tmp = strstr(date_list, cur_date);
    if(tmp != NULL) {
      return true;
    }
  }

  // check start date.
  // if start date is in the future, return false
  tmp_const = json_string_value(json_object_get(j_camp, "sc_date_start"));
  if(tmp_const != NULL) {
    ret = strcmp(tmp_const, cur_date);
    if(ret > 0) {
      return false;
    }
  }

  // check end date.
  // if end date is in the fast, return false
  tmp_const = json_string_value(json_object_get(j_camp, "sc_date_end"));
  if(tmp_const != NULL) {
    ret = strcmp(tmp_const, cur_date);
    if(ret < 0) {
      return false;
    }
  }

  // check startable day
  ret = is_startable_campaign_schedule_day(j_camp, cur_day);
  if(ret == false) {
    return false;
  }

  // check end time
  // if end time is in the fast, return false
  tmp_const = json_string_value(json_object_get(j_camp, "sc_time_end"));
  if(tmp_const != NULL) {
    ret = strcmp(tmp_const, cur_time);
    if(ret < 0) {
      return false;
    }
  }

  // check start time
  // if start time is in the future, return false
  tmp_const = json_string_value(json_object_get(j_camp, "sc_time_start"));
  if(tmp_const != NULL) {
    ret = strcmp(tmp_const, cur_time);
    if(ret > 0) {
      return false;
    }
  }

  return true;
}

static bool is_startable_campaign_schedule_day(json_t* j_camp, int day)
{
  const char* day_list;
  char* tmp;
  char   day_str[2];

  // 0=Sunday, 1=Monday, ..., 6=Saturday
  if((j_camp == NULL) || (day < 0) || (day > 6)) {
    return false;
  }

  // if it doesn't set, just return true.
  day_list = json_string_value(json_object_get(j_camp, "sc_day_list"));
  if((day_list == NULL) || (strlen(day_list) == 0)) {
    return true;
  }

  // check date_list. If there's no current date, return false
  snprintf(day_str, sizeof(day_str), "%d", day);
  tmp = strstr(day_list, day_str);
  if(tmp == NULL) {
    return false;
  }

  return true;
}

static bool is_stopable_campaign_schedule_day_date_list(json_t* j_camp)
{
  char* cur_date;
  char* cur_time;
  int cur_day;
  int ret;

  if(j_camp == NULL) {
    return false;
  }

  cur_date = utils_get_utc_timestamp_date();
  cur_time = utils_get_utc_timestamp_time();
  cur_day = utils_get_utc_timestamp_day();

  // check startable date
  ret = is_stoppable_campaign_schedule_check(j_camp, cur_date, cur_time, cur_day);
  sfree(cur_date);
  sfree(cur_time);
  if(ret == true) {
    return true;
  }

  return false;
}

static bool is_stoppable_campaign_schedule_check(json_t* j_camp, const char* cur_date, const char* cur_time, int cur_day)
{
  int ret;

  ret = is_startable_campaign_schedule_check(j_camp, cur_date, cur_time, cur_day);
  if(ret == true) {
    return false;
  }

  return true;
}

/**
 * Return existence of given ob_campaign uuid.
 * @param uuid
 * @return
 */
bool ob_is_exist_campaign(const char* uuid)
{
  int ret;
  char* sql;
  json_t* j_tmp;

  if(uuid == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return false;
  }

  asprintf(&sql, "select count(*) from ob_campaign where uuid=\"%s\" and in_use=%d;", uuid, E_USE_OK);

  ret = db_ctx_query(g_db_ob, sql);
  sfree(sql);
  if(ret == false) {
    slog(LOG_ERR, "Could not get ob_campaign info. uuid[%s]", uuid);
    return false;
  }

  j_tmp = db_ctx_get_record(g_db_ob);
  db_ctx_free(g_db_ob);
  if(j_tmp == NULL) {
    slog(LOG_ERR, "Could not get correct ob_campaign info. uuid[%s]", uuid);
    return false;
  }

  // result
  ret = json_integer_value(json_object_get(j_tmp, "count(*)"));
  json_decref(j_tmp);
  if(ret <= 0) {
    return false;
  }

  return true;
}

/**
 * Validate given data
 * @param j_data
 * @return
 */
bool ob_validate_campaign(json_t* j_data)
{
  json_t* j_tmp;

  if(j_data == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return false;
  }
  slog(LOG_DEBUG, "Fired validate_ob_campaign.");

  // variables
  j_tmp = json_object_get(j_data, "variables");
  if(j_tmp != NULL) {
    if(json_is_object(j_tmp) != true) {
      slog(LOG_NOTICE, "Wrong input type for variable. It should be json_object type.");
      return false;
    }
  }

  return true;
}

/**
 * Return true if given destination uuid in use in campaign
 * @param uuid_dest
 * @return
 */
bool ob_is_referenced_destination_by_campaign(const char* uuid_dest)
{
  int ret;
  char* sql;
  json_t* j_tmp;

  if(uuid_dest == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return false;
  }
  slog(LOG_DEBUG, "Fired is_referenced_destination_by_campaign. uuid_dest[%s]", uuid_dest);

  asprintf(&sql, "select count(*) from ob_campaign where dest=\"%s\" and in_use=%d;", uuid_dest, E_USE_OK);

  ret = db_ctx_query(g_db_ob, sql);
  sfree(sql);
  if(ret == false) {
    slog(LOG_ERR, "Could not get correct database result.");
    return false;
  }

  j_tmp = db_ctx_get_record(g_db_ob);
  db_ctx_free(g_db_ob);
  if(j_tmp == NULL) {
    return false;
  }

  ret = json_integer_value(json_object_get(j_tmp, "count(*)"));
  json_decref(j_tmp);
  if(ret > 0) {
    return true;
  }

  return true;
}

/**
 * Return true if given dlma uuid in use in campaign
 * @param uuid_dest
 * @return
 */
bool ob_is_referenced_dlma_by_campaign(const char* uuid_dlma)
{
  int ret;
  char* sql;
  json_t* j_tmp;

  if(uuid_dlma == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return false;
  }
  slog(LOG_DEBUG, "Fired is_referenced_dlma_by_campaign. uuid_dlma[%s]", uuid_dlma);

  asprintf(&sql, "select count(*) from ob_campaign where dlma=\"%s\" and in_use=%d;", uuid_dlma, E_USE_OK);

  ret = db_ctx_query(g_db_ob, sql);
  sfree(sql);
  if(ret == false) {
    slog(LOG_ERR, "Could not get correct database result.");
    return false;
  }

  j_tmp = db_ctx_get_record(g_db_ob);
  db_ctx_free(g_db_ob);
  if(j_tmp == NULL) {
    return false;
  }

  ret = json_integer_value(json_object_get(j_tmp, "count(*)"));
  json_decref(j_tmp);
  if(ret > 0) {
    return true;
  }

  return true;
}

/**
 * Return true if given plan uuid in use in campaign
 * @param uuid_dest
 * @return
 */
bool ob_is_referenced_plan_by_campaign(const char* uuid_plan)
{
  char* sql;
  int ret;
  json_t* j_tmp;

  if(uuid_plan == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return false;
  }
  slog(LOG_DEBUG, "Fired is_referenced_plan_by_campaign. uuid_plan[%s]", uuid_plan);

  asprintf(&sql, "select count(*) from ob_campaign where plan=\"%s\" and in_use=%d;", uuid_plan, E_USE_OK);

  ret = db_ctx_query(g_db_ob, sql);
  sfree(sql);
  if(ret == false) {
    slog(LOG_ERR, "Could not get correct database result.");
    return false;
  }

  j_tmp = db_ctx_get_record(g_db_ob);
  db_ctx_free(g_db_ob);
  if(j_tmp == NULL) {
    return false;
  }

  ret = json_integer_value(json_object_get(j_tmp, "count(*)"));
  json_decref(j_tmp);
  if(ret > 0) {
    return true;
  }

  return true;
}


/**
 * Clear destination info from campaign.
 * @param uuid_dest
 * @return
 */
bool ob_clear_campaign_destination(const char* uuid_dest)
{
  char* sql;
  int ret;

  if(uuid_dest == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return false;
  }
  slog(LOG_DEBUG, "Fired clear_campaign_destination. uuid_dest[%s]", uuid_dest);

  asprintf(&sql, "update ob_campaign set dest=null where dest=\"%s\";", uuid_dest);

  ret = db_ctx_exec(g_db_ob, sql);
  sfree(sql);
  if(ret == false) {
    return false;
  }

  return true;
}

/**
 * Clear dlma info from campaign.
 * @param uuid_dest
 * @return
 */
bool ob_clear_campaign_dlma(const char* uuid_dlma)
{
  char* sql;
  int ret;

  if(uuid_dlma == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return false;
  }
  slog(LOG_DEBUG, "Fired clear_campaign_dlma. uuid_dlma[%s]", uuid_dlma);

  asprintf(&sql, "update ob_campaign set dlma=null where dlma=\"%s\";", uuid_dlma);

  ret = db_ctx_exec(g_db_ob, sql);
  sfree(sql);
  if(ret == false) {
    return false;
  }

  return true;
}

/**
 * Clear plan info from campaign.
 * @param uuid_dest
 * @return
 */
bool ob_clear_campaign_plan(const char* uuid_plan)
{
  char* sql;
  int ret;

  if(uuid_plan == NULL) {
    slog(LOG_WARNING, "Wrong input parameter.");
    return false;
  }
  slog(LOG_DEBUG, "Fired clear_campaign_plan. uuid_plan[%s]", uuid_plan);

  asprintf(&sql, "update ob_campaign set plan=null where plan=\"%s\";", uuid_plan);

  ret = db_ctx_exec(g_db_ob, sql);
  sfree(sql);
  if(ret == false) {
    return false;
  }

  return true;
}




