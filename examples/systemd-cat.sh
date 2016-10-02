#!/bin/bash

log_level="info"
if [ "$MESOS_LOG_STREAM" = "STDERR" ]; then
  log_level="err"
fi

task_id=$(echo "$MESOS_EXECUTORINFO_JSON" | \
  jq -r '.command.environment.variables[]|select(.name=="MESOS_TASK_ID").value')

if [ "$task_id" ]; then
    task_identifier="-t $task_id"
fi

systemd-cat $task_identifier -p $log_level
