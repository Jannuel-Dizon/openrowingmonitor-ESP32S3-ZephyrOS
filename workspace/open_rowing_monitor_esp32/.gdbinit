# Connect to the OpenOCD server on your host machine
target extended-remote 172.17.0.1:3333

# Load the symbol table so GDB knows where your functions are in memory
symbol-file build/zephyr/zephyr.elf

# Reset the ESP32 and stop it at the very beginning
monitor reset halt

# Optional: Add a breakpoint at main so it stops where you want to start
# break main
