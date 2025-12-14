#!/bin/bash

# 1. Create a directory for project-specific keys if it doesn't exist
KEY_DIR="$(pwd)/ssh_keys"
KEY_FILE="$KEY_DIR/id_ed25519_zephyr"
mkdir -p "$KEY_DIR"

# 2. Generate a fresh SSH key if one is missing (No password for automation)
if [ ! -f "$KEY_FILE" ]; then
    echo "Generating project-specific SSH keys..."
    ssh-keygen -t ed25519 -f "$KEY_FILE" -N "" -C "zephyr-docker-key"
    chmod 600 "$KEY_FILE"
fi

3. Print "Universal" Connection Info
echo "--------------------------------------------------------"
echo "✅ Container is starting!"
echo ""
echo "To connect via VS Code, Zed, or Terminal, add this to ~/.ssh/config:"
echo ""
echo "Host esp32-dev"
echo "    HostName localhost"
echo "    User root"
echo "    Port 2223"
echo "    IdentityFile $KEY_FILE"
echo "    IdentitiesOnly yes"
echo "    StrictHostKeyChecking no"
echo "    UserKnownHostsFile /dev/null"
echo ""
echo "👉 VS Code: Open 'Remote Explorer', refresh, and click 'esp32-dev'"
echo "👉 Zed: Open 'Remote', and type 'esp32-dev'"
echo "--------------------------------------------------------"
sleep 2

# 4. Run the container
# We mount the PUBLIC key into the container's authorized_keys so it accepts the private key we just generated.
docker run -it --rm \
    --name ESP32ORM_Container \
    --hostname ESP32ORM-dev \
    --privileged \
    -p 2223:22 \
    --device=/dev/ttyUSB0 \
    -v "$(pwd)/workspace:/workspace" \
    -v "$KEY_FILE.pub:/root/.ssh/authorized_keys" \
    -w /workspace \
    esp32orm_zephyr
