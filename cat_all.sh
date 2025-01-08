set +e
docker remove -f filesystem_server
docker remove -f filesystem_client
docker remove -f filesystem_client2
docker remove -f router
set -e
g++ client.cpp -o exe_client/client.out -pthread
g++ server.cpp -o exe_server/server.out -pthread
#sudo docker buildx build . -f ./make_client_image.dockerfile -t filesystem_client
#sudo docker buildx build . -f ./make_server_image.dockerfile -t filesystem_server

docker run -itd --rm --name router router
docker network connect --ip 10.0.10.254 clients router
docker network connect --ip 10.0.11.254 servers router

gnome-terminal -- bash -c "docker run -it --mount type=bind,src=./exe_server,dst=/root/exe --rm --name=filesystem_server --cap-add NET_ADMIN --network=servers --ip 10.0.11.2 filesystem_server; exec bash"
gnome-terminal -- bash -c "docker run -it --mount type=bind,src=./exe_client,dst=/root/exe --rm --name=filesystem_client --cap-add NET_ADMIN --network=clients --ip 10.0.10.2 filesystem_client; exec bash"
gnome-terminal -- bash -c "docker run -it --mount type=bind,src=./exe_client,dst=/root/exe --rm --name=filesystem_client2 --cap-add NET_ADMIN --network=clients --ip 10.0.10.3 filesystem_client; exec bash"