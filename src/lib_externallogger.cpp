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

#include <string>

#include <mesos/mesos.hpp>

#include <mesos/module/container_logger.hpp>

#include <mesos/slave/container_logger.hpp>

#include <process/dispatch.hpp>
#include <process/future.hpp>
#include <process/owned.hpp>
#include <process/process.hpp>
#include <process/subprocess.hpp>

#include <stout/error.hpp>
#include <stout/try.hpp>
#include <stout/nothing.hpp>
#include <stout/option.hpp>
#include <stout/path.hpp>

#include <stout/os/environment.hpp>
#include <stout/os/fcntl.hpp>
#include <stout/os/killtree.hpp>

#include <stout/protobuf.hpp>
#include <stout/stringify.hpp>

#ifdef __linux__
#include <linux/systemd.hpp>
#endif // __linux__

#include "lib_externallogger.hpp"

using namespace mesos;
using namespace process;

using mesos::slave::ContainerLogger;

namespace mesos {
namespace internal {
namespace logger {

using SubprocessInfo = ContainerLogger::SubprocessInfo;

class ExternalContainerLoggerProcess :
  public Process<ExternalContainerLoggerProcess>
{
public:
  ExternalContainerLoggerProcess(const Flags& _flags) : flags(_flags) {}

  // Spawns two subprocesses that should read from stdin to receive the stdout
  // and stderr log streams from the task.
  Future<SubprocessInfo> prepare(
      const ExecutorInfo& executorInfo,
      const std::string& sandboxDirectory,
      const Option<std::string>& user)
  {
    // Setup a blank environment so as not to interfere with the executed
    // subprocess.
    std::map<std::string, std::string> environment;

    // At a minimum we want to put the sandboxDirectory into the environment
    // since it's extremely convenient for shell scripts.
    environment.insert(
        std::pair<std::string, std::string>(flags.mesos_field_prefix +
          "SANDBOX_DIRECTORY", sandboxDirectory)
    );

    // If json protobuf var is not blank, set it into the environment.
    if (flags.executor_info_json_field != "") {
      JSON::Object jsonObjExecInfo = JSON::protobuf(executorInfo);
      std::string jsonExecInfo = stringify(jsonObjExecInfo);
      environment[flags.mesos_field_prefix + flags.executor_info_json_field]
                  = jsonExecInfo;
    }

    // NOTE: We manually construct a pipe here instead of using
    // `Subprocess::PIPE` so that the ownership of the FDs is properly
    // represented.  The `Subprocess` spawned below owns the read-end
    // of the pipe and will be solely responsible for closing that end.
    // The ownership of the write-end will be passed to the caller
    // of this function.
    int pipefd[2];
    if (::pipe(pipefd) == -1) {
      return Failure(ErrnoError("Failed to create pipe").message);
    }

    Subprocess::IO::InputFileDescriptors outfds;
    outfds.read = pipefd[0];
    outfds.write = pipefd[1];

    // NOTE: We need to `cloexec` this FD so that it will be closed when
    // the child subprocess is spawned and so that the FD will not be
    // inherited by the second child for stderr.
    Try<Nothing> cloexec = os::cloexec(outfds.write.get());
    if (cloexec.isError()) {
      os::close(outfds.read);
      os::close(outfds.write.get());
      return Failure("Failed to cloexec: " + cloexec.error());
    }

    // If we are on systemd, then extend the life of the process as we
    // do with the executor. Any grandchildren's lives will also be
    // extended.
    std::vector<Subprocess::ParentHook> parentHooks;
#ifdef __linux__
    if (systemd::enabled()) {
      parentHooks.emplace_back(Subprocess::ParentHook(
          &systemd::mesos::extendLifetime));
    }
#endif // __linux__

    // Each logger needs to SETSID so it can continue logging if the agent
    // dies.
    std::vector<Subprocess::ChildHook> childHooks;
    childHooks.emplace_back(Subprocess::ChildHook::SETSID());

    // Set the stream name for the stdout stream
    environment[flags.mesos_field_prefix + flags.stream_name_field] = "STDOUT";

    Try<Subprocess> outProcess = subprocess(
        flags.external_logger_binary,
        std::vector<std::string>{std::string(flags.external_logger_binary)},
        Subprocess::FD(outfds.read, Subprocess::IO::OWNED),
        Subprocess::PATH("/dev/null"),
        Subprocess::FD(STDERR_FILENO),
        nullptr,
        environment,
        None(),
        parentHooks,
        childHooks);

    if (outProcess.isError()) {
      os::close(outfds.write.get());
      return Failure("Failed to create logger process: " +
                     outProcess.error());
    }

    // NOTE: We manually construct a pipe here to properly express
    // ownership of the FDs.  See the NOTE above.
    if (::pipe(pipefd) == -1) {
      os::close(outfds.write.get());
      os::killtree(outProcess.get().pid(), SIGKILL);
      return Failure(ErrnoError("Failed to create pipe").message);
    }

    Subprocess::IO::InputFileDescriptors errfds;
    errfds.read = pipefd[0];
    errfds.write = pipefd[1];

    // NOTE: We need to `cloexec` this FD so that it will be closed when
    // the child subprocess is spawned.
    cloexec = os::cloexec(errfds.write.get());
    if (cloexec.isError()) {
      os::close(outfds.write.get());
      os::close(errfds.read);
      os::close(errfds.write.get());
      os::killtree(outProcess.get().pid(), SIGKILL);
      return Failure("Failed to cloexec: " + cloexec.error());
    }

    // Set the stream name for the stderr stream
    environment[flags.mesos_field_prefix + flags.stream_name_field] = "STDERR";

    Try<Subprocess> errProcess = subprocess(
        flags.external_logger_binary,
        std::vector<std::string>{flags.external_logger_binary},
        Subprocess::FD(errfds.read, Subprocess::IO::OWNED),
        Subprocess::PATH("/dev/null"),
        Subprocess::FD(STDERR_FILENO),
        nullptr,
        environment,
        None(),
        parentHooks,
        childHooks);

    if (errProcess.isError()) {
      os::close(outfds.write.get());
      os::close(errfds.write.get());
      os::killtree(outProcess.get().pid(), SIGKILL);
      return Failure("Failed to create logger process: " +
          errProcess.error());
    }

    // The ownership of these FDs is given to the caller of this function.
    ContainerLogger::SubprocessInfo info;
    info.out = SubprocessInfo::IO::FD(outfds.write.get());
    info.err = SubprocessInfo::IO::FD(errfds.write.get());
    return info;
  }
protected:
    Flags flags;
};


ExternalContainerLogger::ExternalContainerLogger(const Flags& _flags)
  : flags(_flags),
    process(new ExternalContainerLoggerProcess(flags))
{
  spawn(process.get());
}


ExternalContainerLogger::~ExternalContainerLogger()
{
  terminate(process.get());
  wait(process.get());
}


Try<Nothing> ExternalContainerLogger::initialize()
{
  return Nothing();
}

Future<ContainerLogger::SubprocessInfo>
ExternalContainerLogger::prepare(
    const ExecutorInfo& executorInfo,
    const std::string& sandboxDirectory,
    const Option<std::string>& user)
{
  return dispatch(
      process.get(),
      &ExternalContainerLoggerProcess::prepare,
      executorInfo,
      sandboxDirectory,
      user);
}

} // namespace logger {
} // namespace internal {
} // namespace mesos {

mesos::modules::Module<ContainerLogger>
org_apache_mesos_ExternalContainerLogger(
    MESOS_MODULE_API_VERSION,
    MESOS_VERSION,
    "Apache Mesos",
    "modules@mesos.apache.org",
    "External Process Logger module.",
    nullptr,
    [](const Parameters& parameters) -> ContainerLogger* {
      // Convert `parameters` into a map.
      std::map<std::string, std::string> values;
      foreach (const Parameter& parameter, parameters.parameter()) {
        values[parameter.key()] = parameter.value();
      }

      // Load and validate flags from the map.
      mesos::internal::logger::Flags flags;
      Try<flags::Warnings> load = flags.load(values);

      if (load.isError()) {
        LOG(ERROR) << "Failed to parse parameters: " << load.error();
        return nullptr;
      }

      // Log any flag warnings.
      foreach (const flags::Warning& warning, load->warnings) {
        LOG(WARNING) << warning.message;
      }

      return new mesos::internal::logger::ExternalContainerLogger(flags);
    });
