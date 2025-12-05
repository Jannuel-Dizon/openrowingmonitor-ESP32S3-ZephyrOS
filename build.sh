#!/bin/bash

docker build \
    -t esp32orm_zephyr \
    -f Dockerfile \
    .

ssh-keygen -f '/home/jannnuel-dizon/.ssh/known_hosts' -R '[localhost]:2223'
