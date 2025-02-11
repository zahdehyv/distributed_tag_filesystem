FROM alpine_gcc

WORKDIR /root
USER root
RUN mkdir exe

ENTRYPOINT ["./exe/dht_server.out"]