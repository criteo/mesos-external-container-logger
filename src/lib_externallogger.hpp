// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __SLAVE_CONTAINER_LOGGERS_EXTERNAL_HPP__
#define __SLAVE_CONTAINER_LOGGERS_EXTERNAL_HPP__

#include <string>

#include <mesos/mesos.hpp>

#include <mesos/slave/container_logger.hpp>

#include <process/future.hpp>
#include <process/owned.hpp>
#include <process/subprocess.hpp>

#include <stout/try.hpp>
#include <stout/nothing.hpp>
#include <stout/option.hpp>

#include <stout/os/exists.hpp>

namespace mesos {
namespace internal {
namespace logger {

// Forward declaration.
class ExternalContainerLoggerProcess;


struct Flags : public virtual flags::FlagsBase
{
  Flags()
  {
    add(&Flags::external_logger_binary,
        "external_logger_binary",
        "external_logger_script",
        "Path to the external command which will read STDIN for logs",
        static_cast<const std::string*>(nullptr),
        [](const std::string& executablePath) -> Option<Error> {
          if (!os::exists(executablePath)) {
              return Error("Cannot find: " + executablePath);
          }

          // Check the specified command is an executable.
          Try<bool> isExecutable = os::access(executablePath, X_OK);
          if (isExecutable.isSome()) {
              if (!isExecutable.get()) {
                  return Error("Not executable by Mesos: " + executablePath);
              }
          } else {
              return Error("Cannot stat for access check: " + executablePath);
          }

          return None();
        });

    add(&Flags::mesos_field_prefix,
        "mesos_field_prefix",
        "Prefix to add to environment variables containing task data passed\n"
        "to the external logger process.",
        "MESOS_LOG_");

    add(&Flags::stream_name_field,
        "stream_name_field",
        "Name of the field to store the stdout/stderr stream identifier under",
        "STREAM");

    add(&Flags::executor_info_json_field,
        "executor_info_json_field",
        "Name of the environment variable to store JSON serialization of \n"
        "executorInfo under. Note this field is also concatenated with \n"
        "the value of --mesos_field_prefix.",
        "EXECUTORINFO_JSON");
  }

  std::string external_logger_binary;
  std::string mesos_field_prefix;
  std::string stream_name_field;
  std::string executor_info_json_field;
};

// The external process container logger.
//
// Executors and tasks launched through this container logger will have their
// stdout and stderr piped to the specified external logger, launched with
// metadata in environment variables.
class ExternalContainerLogger : public mesos::slave::ContainerLogger
{
public:
  ExternalContainerLogger(const Flags& _flags);

  virtual ~ExternalContainerLogger();

  // This is a noop.  The external container logger has nothing to initialize.
  virtual Try<Nothing> initialize();

  // Tells the subprocess to redirect the executor/task's stdout and stderr
  // to the external command.
  virtual process::Future<mesos::slave::ContainerLogger::SubprocessInfo>
  prepare(
      const ExecutorInfo& executorInfo,
      const std::string& sandboxDirectory,
      const Option<std::string>& user);

protected:
  Flags flags;
  process::Owned<ExternalContainerLoggerProcess> process;
};


} // namespace logger {
} // namespace internal {
} // namespace mesos {

#endif // __SLAVE_CONTAINER_LOGGERS_EXTERNAL_HPP__
