#pragma once

#include "webapp-privt.h"

int webapp_task_read(webapp_thread_t* t);
int webapp_task_process(webapp_thread_t* t);
int webapp_task_write(webapp_thread_t* t);
