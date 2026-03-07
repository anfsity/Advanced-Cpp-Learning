#!/bin/bash
IMAGE="vsavkov/clang-p2996:amd64"

docker run -it --rm \
    -u $(id -u):$(id -g) \
    -v "$(pwd):$(pwd)" \
    -w "$(pwd)" \
    "$IMAGE" bash
