FROM alpine_gcc

USER root
RUN mkdir root/exe
WORKDIR /root

COPY ./infra/client.sh /root
RUN chmod +x ./client.sh

ENTRYPOINT ["sh", "./client.sh"]