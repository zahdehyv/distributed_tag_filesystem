sudo docker buildx build . -f ./make_client_image.dockerfile -t example_client
sudo docker buildx build . -f ./make_server_image.dockerfile -t example_server