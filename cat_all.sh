set +e
sh close_all.sh

rm -r ./exe_client/dload
mkdir ./exe_client/dload

set -e
g++ chord_server_main.cpp -o exe_dht_server/dht_server.out -pthread
g++ client.cpp -o exe_client/client.out -pthread
g++ server.cpp -o exe_server/server.out -pthread
#sudo docker buildx build . -f ./make_client_image.dockerfile -t filesystem_client
#sudo docker buildx build . -f ./make_server_image.dockerfile -t filesystem_server
#sudo docker buildx build . -f ./make_dht_server_image.dockerfile -t dht_server

sh turn_router_on.sh

#gnome-terminal -- bash -c "docker run -it --mount type=bind,src=./exe_server,dst=/root/exe --rm --name=filesystem_server0 --cap-add NET_ADMIN --network=servers --ip 10.0.11.2 filesystem_server; exec bash"
gnome-terminal -- bash -c "docker run -it --mount type=bind,src=./exe_server,dst=/root/exe --rm --name=filesystem_server1 --cap-add NET_ADMIN --network=servers --ip 10.0.11.231 filesystem_server; exec bash"

gnome-terminal -- bash -c "docker run -it --mount type=bind,src=./exe_client,dst=/root/exe --rm --name=filesystem_client0 --cap-add NET_ADMIN --network=clients --ip 10.0.10.2 filesystem_client; exec bash"
#gnome-terminal -- bash -c "docker run -it --mount type=bind,src=./exe_client,dst=/root/exe --rm --name=filesystem_client1 --cap-add NET_ADMIN --network=clients --ip 10.0.10.3 filesystem_client; exec bash"

gnome-terminal -- bash -c "docker run -it --mount type=bind,src=./exe_dht_server,dst=/root/exe --rm --name=tags_to_files_dht_server0 --cap-add NET_ADMIN --network=servers dht_server tags_files 239.193.7.77 4096 Y 10; exec bash"
#gnome-terminal -- bash -c "docker run --mount type=bind,src=./exe_dht_server,dst=/root/exe -it --rm --name=tags_to_files_dht_server1 --cap-add NET_ADMIN --network=servers dht_server tags_files 239.193.7.77 4096 N 18; exec bash"
#gnome-terminal -- bash -c "docker run --mount type=bind,src=./exe_dht_server,dst=/root/exe -it --rm --name=tags_to_files_dht_server2 --cap-add NET_ADMIN --network=servers dht_server tags_files 239.193.7.77 4096 N 13; exec bash"

gnome-terminal -- bash -c "docker run -it --mount type=bind,src=./exe_dht_server,dst=/root/exe --rm --name=files_dht_server0 --cap-add NET_ADMIN --network=servers dht_server files 239.193.7.78 4097 Y 10; exec bash"
#gnome-terminal -- bash -c "docker run -it --mount type=bind,src=./exe_dht_server,dst=/root/exe --rm --name=files_dht_server1 --cap-add NET_ADMIN --network=servers dht_server files 239.193.7.78 4097 N 18; exec bash"
#gnome-terminal -- bash -c "docker run -it --mount type=bind,src=./exe_dht_server,dst=/root/exe  --rm --name=files_dht_server2 --cap-add NET_ADMIN --network=servers dht_server files 239.193.7.78 4097 N 13; exec bash"

gnome-terminal -- bash -c "docker run -it --mount type=bind,src=./exe_dht_server,dst=/root/exe --rm --name=files_to_tags_dht_server0 --cap-add NET_ADMIN --network=servers dht_server files_tags 239.193.7.79 4098 Y 10; exec bash"
#gnome-terminal -- bash -c "docker run -it --mount type=bind,src=./exe_dht_server,dst=/root/exe --rm --name=files_to_tags_dht_server1 --cap-add NET_ADMIN --network=servers dht_server files_tags 239.193.7.79 4098 N 18; exec bash"
#gnome-terminal -- bash -c "docker run -it --mount type=bind,src=./exe_dht_server,dst=/root/exe  --rm --name=files_to_tags_dht_server2 --cap-add NET_ADMIN --network=servers dht_server files_tags 239.193.7.79 4098 N 13; exec bash"
