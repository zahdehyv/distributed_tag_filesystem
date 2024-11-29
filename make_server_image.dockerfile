FROM alpine_gcc
COPY ./server.out /root
WORKDIR /root
USER root
RUN mkdir files
RUN mkdir file_tags
ENTRYPOINT ["./server.out"]