FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    python3 \
    python3-pip \
    wget \
    unzip \
    openocd \
    minicom \
    tmux \
    gcc-arm-none-eabi \
    gdb-multiarch \
    usbutils \
    udev \
    && rm -rf /var/lib/apt/lists/*

RUN usermod -aG dialout,plugdev vscode

WORKDIR /workspace