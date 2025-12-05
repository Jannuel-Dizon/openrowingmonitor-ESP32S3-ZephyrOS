#!/bin/bash

docker run -it --rm \
    --name ESP32ORM_Container \
    --hostname ESP32ORM-dev \
    --privileged \
    -p 2223:22 \
    --device=/dev/ttyUSB0 \
    -v "$(pwd)/workspace:/workspace" \
    -w /workspace \
    esp32orm_zephyr
