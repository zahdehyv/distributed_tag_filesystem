FROM alpine_gcc
COPY ./client.out /root
WORKDIR /root
USER root
ENTRYPOINT ["./client.out"]