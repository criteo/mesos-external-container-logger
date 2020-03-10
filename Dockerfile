FROM criteo/mesosbuild:1.9.0-automake

ENV MESOS_BUILD_DIR=/src/mesos
ENV CONTAINER_LOGGER_DIR=/src/mesos-external-container-logger
WORKDIR $MESOS_BUILD_DIR
ADD . $CONTAINER_LOGGER_DIR

RUN make DESTDIR=$CONTAINER_LOGGER_DIR/mesos-install install
RUN mkdir build
WORKDIR build
RUN ln -s $MESOS_BUILD_DIR/3rdparty
WORKDIR $CONTAINER_LOGGER_DIR
RUN ln -s $MESOS_BUILD_DIR mesos
RUN mkdir build
WORKDIR build
ENV CXXFLAGS="-isystem $MESOS_BUILD_DIR/3rdparty/protobuf-*/src -isystem $MESOS_BUILD_DIR/3rdparty/glog-*/src -isystem $MESOS_BUILD_DIR/3rdparty/boost-*[0-9] -isystem $CONTAINER_LOGGER_DIR/mesos-install/usr/include -isystem $CONTAINER_LOGGER_DIR/mesos-install/usr/lib64/mesos/3rdparty/usr/include"
RUN cmake3 -DWITH_MESOS=$CONTAINER_LOGGER_DIR/mesos-install/usr/local/ -DMESOS_SRC_DIR=$CONTAINER_LOGGER_DIR/mesos ..
RUN make -j 3
