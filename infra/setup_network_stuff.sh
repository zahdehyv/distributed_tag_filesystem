docker network create clients --subnet 10.0.10.0/24
docker network create servers --subnet 10.0.11.0/24
docker build -t router -f ./router.Dockerfile .