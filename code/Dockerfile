FROM ubuntu:18.04
RUN apt-get update && apt-get -y install g++ make
RUN mkdir /var/log/erss
RUN mkdir /code
ADD . /code
WORKDIR /code

CMD ["./proxy"]