FROM alpine_gcc
COPY ./server.out /root
WORKDIR /root
USER root
ENTRYPOINT ["./server.out"]