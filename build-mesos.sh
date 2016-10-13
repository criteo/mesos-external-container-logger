#!/bin/bash

set -e

MESOS_VERSION=1.0.1

if [ ! -f mesos-$MESOS_VERSION.tar.gz ]; then
    curl --remote-name http://www-eu.apache.org/dist/mesos/$MESOS_VERSION/mesos-$MESOS_VERSION.tar.gz
fi

if [ ! -d mesos-$MESOS_VERSION ]; then
    tar xf mesos-$MESOS_VERSION.tar.gz
fi

cd mesos-$MESOS_VERSION
if [ ! -d build ]; then
    mkdir build
fi

#sudo apt-get -y install curl libz-dev libsystemd-dev
#sudo apt-get -y install build-essential python-dev libcurl4-nss-dev libsasl2-dev libsasl2-modules maven libapr1-dev libsvn-dev openjdk-8-jdk-headless openjdk-8-jre-headless cmake

if [ ! -d ../mesos-install ]; then
  cd build
  ../configure CXXFLAGS='-O2 -Wno-deprecated-declarations' \
               --prefix=$PWD/../../mesos-install --enable-install-module-dependencies
  make -j 6 V=0
  make install
fi
