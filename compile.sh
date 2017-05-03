#!/bin/bash

set -e

cmake -DWITH_MESOS=$PWD/mesos-install -DMESOS_SRC_DIR=$PWD/mesos-1.2.0
make -j 6 V=0
