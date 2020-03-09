# mesos-external-container-logger

This is a container logger module for [Mesos](http://mesos.apache.org/)
that redirects container logs to an external process.

It is a port to Mesos 1.7+ of the `ExternalContainerLogger` module
developed in
[MESOS-6003](https://issues.apache.org/jira/browse/MESOS-6003).
Read the [documentation](https://reviews.apache.org/r/51258/) or this
[blog post](https://wjoel.com/posts/mesos-container-log-forwarding-with-filebeat.html)
for more information and some example use cases.

The module has been compiled for Debian 8 (jessie) and Debian testing (buster).
Use the `compile.sh` and optionally the
`build-mesos.sh` scripts to build the module for other distributions.
CMake 3.0.0 or later is required to build.

# Credits

Thanks @wjoel for the creation and sheparding of this project.
