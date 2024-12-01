FROM alpine_gcc
COPY ./server.out /root
WORKDIR /root
USER root

COPY ./infra/server.sh /root
RUN chmod +x ./server.sh

RUN mkdir files
RUN mkdir file_tags
ENTRYPOINT ["sh", "./server.sh"]