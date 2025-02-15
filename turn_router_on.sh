docker run -itd --rm --name router router
docker network connect --ip 10.0.10.254 clients router
docker network connect --ip 10.0.11.254 servers router