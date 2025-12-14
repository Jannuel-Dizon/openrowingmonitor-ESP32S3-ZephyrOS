# CHANGED: Generic Zephyr development image for all supported targets.
# Optimized for a generic SSH-based development workflow (e.g., with Zed)

# Settings
ARG DEBIAN_VERSION=stable-20241016-slim
ARG PASSWORD="zephyr"
ARG ZEPHYR_RTOS_VERSION=4.1.0
ARG ZEPHYR_RTOS_COMMIT=v4.1.0
ARG ZEPHYR_SDK_VERSION=0.17.1
ARG WGET_ARGS="-q --show-progress --progress=bar:force:noscroll"
ARG VIRTUAL_ENV=/opt/venv

#-------------------------------------------------------------------------------
# Base Image and Dependencies

# Use Debian as the base image
FROM debian:${DEBIAN_VERSION}

# Redeclare arguments after FROM
ARG PASSWORD
ARG ZEPHYR_RTOS_COMMIT
ARG ZEPHYR_SDK_VERSION
ARG ZEPHYR_RTOS_VERSION
# REMOVED: TOOLCHAIN_LIST
ARG WGET_ARGS
ARG VIRTUAL_ENV
ARG TARGETARCH

# Set default shell during Docker image build to bash
SHELL ["/bin/bash", "-c"]

# Check if the target architecture is either x86_64 (amd64) or arm64 (aarch64)
RUN if [ "$TARGETARCH" = "amd64" ]; then \
    export HOST_ARCH="x86_64"; \
    elif [ "$TARGETARCH" = "arm64" ]; then \
    export HOST_ARCH="aarch64"; \
    else \
    echo "Unsupported architecture: $TARGETARCH"; \
    exit 1; \
    fi && \
    echo "Architecture $TARGETARCH is supported."

# Set non-interactive frontend for apt-get to skip any user confirmations
ENV DEBIAN_FRONTEND=noninteractive

# Install base packages
# Note: openssh-server is kept as it's essential for the Zed SSH workflow
RUN apt-get -y update && \
    apt-get install --no-install-recommends -y \
    dos2unix \
    ca-certificates \
    file \
    locales \
    git \
    build-essential \
    cmake \
    ninja-build gperf \
    device-tree-compiler \
    wget \
    curl \
    python3 \
    python3-pip \
    python3-venv \
    xz-utils \
    dos2unix \
    vim \
    nano \
    mc \
    openssh-server \
    clangd

# Set root password
RUN echo "root:${PASSWORD}" | chpasswd

# Set up a Python virtual environment
ENV VIRTUAL_ENV=${VIRTUAL_ENV}
RUN python3 -m venv ${VIRTUAL_ENV}
ENV PATH="${VIRTUAL_ENV}/bin:$PATH"

# Install west
RUN python3 -m pip install --no-cache-dir west

# Clean up stale packages
RUN apt-get clean -y && \
    apt-get autoremove --purge -y && \
    rm -rf /var/lib/apt/lists/*

# Set up directories
RUN mkdir -p /workspace/ && \
    mkdir -p /opt/toolchains

# Set up sshd working directory
RUN mkdir -p /var/run/sshd && \
    chmod 0755 /var/run/sshd

# Allow root login via SSH
RUN sed -i 's/#PermitRootLogin prohibit-password/PermitRootLogin yes/' /etc/ssh/sshd_config && \
    sed -i 's/#PasswordAuthentication yes/PasswordAuthentication yes/' /etc/ssh/sshd_config && \
    sed -i 's/#StrictModes yes/StrictModes no/' /etc/ssh/sshd_config

# Expose SSH port
EXPOSE 22

#-------------------------------------------------------------------------------
# Zephyr RTOS

# Set Zephyr environment variables
ENV ZEPHYR_RTOS_VERSION=${ZEPHYR_RTOS_VERSION}

# Install Zephyr
RUN cd /opt/toolchains && \
    git clone https://github.com/zephyrproject-rtos/zephyr.git && \
    cd zephyr && \
    git checkout ${ZEPHYR_RTOS_COMMIT} && \
    python3 -m pip install -r scripts/requirements-base.txt

# NOTE: We are NOT copying a custom west.yml. This ensures we get the full,
# standard set of Zephyr modules for broad compatibility.

# Instantiate west workspace and install all modules
RUN cd /opt/toolchains && \
    west init -l zephyr && \
    west update --narrow -o=--depth=1

# NOTE: The "west blobs fetch" command for specific modules is removed.
# If a project requires blobs (like hal_espressif), the user should fetch them
# within their own workspace, not in the base Docker image.

#-------------------------------------------------------------------------------
# Zephyr SDK

# Set environment variables
ENV ZEPHYR_SDK_VERSION=${ZEPHYR_SDK_VERSION}

# CHANGED: Install the COMPLETE Zephyr SDK, not the minimal one.
# This single package contains all toolchains (ARM, RISC-V, etc.) and host tools.
# RUN cd /opt/toolchains && \
#     # Determine arch for SDK URL. Docker's $TARGETARCH gives amd64/arm64.
#     if [ "$TARGETARCH" = "amd64" ]; then export SDK_ARCH="x86_64"; else export SDK_ARCH="aarch64"; fi && \
#     # Download the full SDK bundle
#     wget ${WGET_ARGS} https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v${ZEPHYR_SDK_VERSION}/zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-${SDK_ARCH}.tar.xz && \
#     # Extract the SDK
#     tar xf zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-${SDK_ARCH}.tar.xz && \
#     rm zephyr-sdk-${ZEPHYR_SDK_VERSION}_linux-${SDK_ARCH}.tar.xz && \
#     # Run the setup script to register the SDK with Zephyr
#     cd zephyr-sdk-${ZEPHYR_SDK_VERSION} && \
#     bash setup.sh

RUN cd /opt/toolchains && \
    west sdk install

# REMOVED: The complex, multi-step installation of minimal SDK + toolchains + host tools
# is now replaced by the single, simpler block above.

#-------------------------------------------------------------------------------
# Optional Settings

# Initialise system locale (required by menuconfig)
RUN sed -i '/^#.*en_US.UTF-8/s/^#//' /etc/locale.gen && \
    locale-gen en_US.UTF-8 && \
    update-locale LANG=en_US.UTF-8 LC_ALL=en_US.UTF-8

# Use the "dark" theme for Midnight Commander
ENV MC_SKIN=dark

#-------------------------------------------------------------------------------
# Entrypoint

# Activate the Python and Zephyr environments for shell sessions
RUN echo "source ${VIRTUAL_ENV}/bin/activate" >> /root/.bashrc && \
    echo "source /opt/toolchains/zephyr/zephyr-env.sh" >> /root/.bashrc && \
    # This makes the SDK available in the path
    echo "export ZEPHYR_TOOLCHAIN_VARIANT=zephyr" >> /root/.bashrc && \
    echo "export ZEPHYR_SDK_INSTALL_DIR=/opt/toolchains/zephyr-sdk-${ZEPHYR_SDK_VERSION}" >> /root/.bashrc

# Set working directory for the user
COPY scripts/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh && \
    dos2unix /entrypoint.sh
ENTRYPOINT ["/entrypoint.sh"]
