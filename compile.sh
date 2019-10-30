#!/bin/bash

set -e

cmake -DWITH_MESOS=$PWD/mesos-install -DMESOS_SRC_DIR=$PWD/mesos-1.7.2
make -j 6 V=0
