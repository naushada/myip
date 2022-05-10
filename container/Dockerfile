FROM ubuntu:focal
ENV TZ=Asia/Calcutta
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get -y upgrade
RUN apt-get install -y --no-install-recommends \
    ca-cacert \
    cmake \
    build-essential \
    libboost-all-dev \
    libssl-dev \
    wget \
    zlib1g-dev

RUN apt-get update
RUN apt-get -y upgrade
RUN apt-get -y install libboost-all-dev
RUN apt-get -y install libbson-dev
RUN apt-get -y install libzstd-dev
RUN apt-get -y install git

# get and build ACE
WORKDIR /root
RUN wget https://github.com/DOCGroup/ACE_TAO/releases/download/ACE%2BTAO-7_0_0/ACE+TAO-7.0.0.tar.gz
RUN tar -xzvf ACE+TAO-7.0.0.tar.gz
ENV ACE_SRC=/root/ACE_wrappers ACE_PREFIX=/usr/local/ACE_TAO-7.0.0
RUN echo '#include "ace/config-linux.h"' > ACE_wrappers/ace/config.h
RUN echo 'include $(ACE_SRC)/include/makeinclude/platform_linux.GNU' > $ACE_SRC/include/makeinclude/platform_macros.GNU
WORKDIR /root/ACE_wrappers
RUN make install ssl=1 INSTALL_PREFIX=${ACE_PREFIX} ACE_ROOT=${ACE_SRC} SSL_ROOT=/usr/include/opensll
RUN ldconfig

#Building uniip - to get my dynamic IP address
WORKDIR /root
RUN git clone https://github.com/naushada/myip.git
RUN cd myip
RUN mkdir ix86_64
WORKDIR /root/myip/ix86_64
RUN cmake .. && make

WORKDIR /opt/xAPP
RUN mkdir myip
WORKDIR /opt/xAPP/myip

# copy from previoud build stage
RUN cp /root/myip/ix86_64/uniip .

# CMD_ARGS will be : --ip <ip> --port <port>

ENV PORT=8080
CMD "/opt/xAPP/myip/uniip" --port ${PORT} ${ARGS}
