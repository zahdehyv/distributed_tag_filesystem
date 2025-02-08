FROM alpine_gcc

USER root
WORKDIR /root

COPY ./client.out /root

ENTRYPOINT ["./client.out"]