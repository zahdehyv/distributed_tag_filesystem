FROM alpine_gcc
COPY ./client.out /root
WORKDIR /root
USER root
COPY ./example_files/a.txt /root
COPY ./example_files/b.cpp /root
COPY ./example_files/c.py /root
ENTRYPOINT ["./client.out"]