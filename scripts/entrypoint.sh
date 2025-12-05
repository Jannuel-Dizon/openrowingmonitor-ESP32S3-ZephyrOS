#!/bin/bash

# Start the SSH daemon in the background
/usr/sbin/sshd

# Keep the container running
# This command will wait forever, allowing the sshd background process to keep running.
# If the script exits, the container will stop.
exec tail -f /dev/null
