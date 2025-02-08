set +e
sh close_all.sh

set -e
g++ client_main.cpp -o client.out -pthread
g++ chord_server_main.cpp -o server.out -pthread

sudo docker buildx build . -f ./make_client_image.dockerfile -t dht_client
sudo docker buildx build . -f ./make_server_image.dockerfile -t dht_server

gnome-terminal -- bash -c "docker run -it --rm --name=dht_server0 --cap-add NET_ADMIN --network=dht_test dht_server files 239.193.7.77 4096 226.0.0.2 2020 Y; exec bash"
gnome-terminal -- bash -c "docker run -it --rm --name=dht_server1 --cap-add NET_ADMIN --network=dht_test dht_server files 239.193.7.77 4096 226.0.0.2 2020 N; exec bash"
#gnome-terminal -- bash -c "docker run -it --rm --name=dht_server2 --cap-add NET_ADMIN --network=dht_test dht_server files 239.193.7.77 4096 226.0.0.2 2020 N; exec bash"
#gnome-terminal -- bash -c "docker run -it --rm --name=dht_client0 --cap-add NET_ADMIN --network=host dht_client; exec bash"
