set -e
g++ client.cpp -o client.out
g++ server.cpp -o server.out
sudo docker buildx build . -f ./make_client_image.dockerfile -t filesystem_client
sudo docker buildx build . -f ./make_server_image.dockerfile -t filesystem_server
set +e
docker remove -f filesystem_server
docker remove -f filesystem_client
set -e
gnome-terminal -- bash -c "docker run -i --rm --name=filesystem_server --network=example_network filesystem_server; exec bash"
gnome-terminal -- bash -c "docker run -i --rm --name=filesystem_client --network=example_network filesystem_client; exec bash"
