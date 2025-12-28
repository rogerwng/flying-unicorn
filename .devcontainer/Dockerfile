FROM mcr.microsoft.com/devcontainers/base:ubuntu-22.04

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
    && rm -rf /var/lib/apt/lists/*

# ARM GCC (xPack, ARM64)
RUN curl -L -o /tmp/arm-gcc.tar.gz \
 https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases/download/v14.2.1-1.1/xpack-arm-none-eabi-gcc-14.2.1-1.1-linux-arm64.tar.gz \
 && tar -xzf /tmp/arm-gcc.tar.gz -C /opt \
 && rm /tmp/arm-gcc.tar.gz

ENV PATH="/opt/xpack-arm-none-eabi-gcc-14.2.1-1.1/bin:${PATH}"

WORKDIR /workspace