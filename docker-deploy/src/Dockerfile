FROM ubuntu:18.04
RUN apt-get update && apt-get install -y g++
RUN mkdir /var/log/erss
RUN mkdir /code
COPY . /code
WORKDIR /code