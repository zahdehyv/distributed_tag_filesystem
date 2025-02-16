docker remove -f filesystem_client0
docker remove -f filesystem_client1
docker remove -f filesystem_client2

docker remove -f filesystem_server0
docker remove -f filesystem_server1
docker remove -f filesystem_server2

docker remove -f router

docker remove -f tags_to_files_dht_server0
docker remove -f tags_to_files_dht_server1
docker remove -f tags_to_files_dht_server2
docker remove -f tags_to_files_dht_server3

docker remove -f files_dht_server0
docker remove -f files_dht_server1
docker remove -f files_dht_server2
docker remove -f files_dht_server3

docker remove -f files_to_tags_dht_server0
docker remove -f files_to_tags_dht_server1
docker remove -f files_to_tags_dht_server2
docker remove -f files_to_tags_dht_server3

rm -r ./exe_client/dload