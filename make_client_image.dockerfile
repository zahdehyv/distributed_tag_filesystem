FROM alpine_gcc
COPY ./client.out /root
WORKDIR /root
USER root
COPY ./infra/client.sh /root
RUN chmod +x ./client.sh

COPY ./example_files/a.txt /root
COPY ./example_files/b.cpp /root
COPY ./example_files/c.py /root
COPY ./example_files/Test.zip /root

ENTRYPOINT ["sh", "./client.sh"]