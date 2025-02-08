FROM alpine_gcc

USER root
WORKDIR /root

COPY ./server.out /root

ENTRYPOINT ["./server.out"]