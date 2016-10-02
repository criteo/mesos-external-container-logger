#!/bin/bash
# This creates a config file for Filebeat in the sandbox directory
# and starts Filebeat, reading from standard input.

# We use the logstash output by default, which Graylog supports
# through the Beats input.
OUTPUT_HOSTS='["your.graylog.example.com:5044"]'

config_path="$MESOS_LOG_SANDBOX_DIRECTORY/filebeat-$MESOS_LOG_STREAM.yml"

mesos_fields=$(echo "$MESOS_EXECUTORINFO_JSON" | \
                jq -r ".command.environment.variables
                         |map(\"\(.name):\(.value|tostring)\")|.[]" | \
                # Skip empty variables, use mesos_ prefix, convert to lowercase
                awk -F: 'length($2) > 0 {
                           $1=tolower($1);
                           if (!match($1, "^mesos_.*")) {
                             $1="mesos_" $1;
                           }
                           printf("%s: \"%s\"\n        ", $1, $2);
                         }')

cat <<EOF > $config_path
filebeat:
  prospectors:
    -
      paths:
        - "-"
      input_type: stdin
      close_eof: true
      fields:
        mesos_log_stream: $MESOS_LOG_STREAM
        mesos_log_sandbox_directory: $MESOS_LOG_SANDBOX_DIRECTORY
        $mesos_fields

output:
  logstash:
    hosts: $OUTPUT_HOSTS
EOF

/usr/bin/filebeat -c $config_path
