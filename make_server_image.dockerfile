FROM alpine_gcc

WORKDIR /root
USER root
RUN mkdir exe
RUN mkdir files
RUN mkdir file_tags

COPY ./infra/server.sh /root
RUN chmod +x ./server.sh

ENTRYPOINT ["sh", "./server.sh"]