FROM ubuntu:18.04
USER root

RUN apt-get update; \
    apt -y install g++; \
    apt-get -y install cmake; \
    apt-get -y install libboost-all-dev; \
    apt-get -y install libevent-dev;

WORKDIR /server
COPY . /server

COPY ./httpd.conf /etc/httpd.conf
#COPY ./httptest /var/www/html/httptest

RUN cmake .
RUN make

EXPOSE 80

CMD ./Server "/etc/httpd.conf"