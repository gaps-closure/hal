FROM ubuntu:19.10

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && \
    apt-get -y install gcc mono-mcs && \
    apt-get -y install make && \
    apt-get -y install iproute2 && \
    apt-get -y install python3 && \    
    apt-get -y install netcat && \        
    apt-get -y install libzmq3-dev && \
    apt-get -y install libconfig-dev && \    
    rm -rf /var/lib/apt/lists/*

USER root