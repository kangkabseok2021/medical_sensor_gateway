FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    cmake \
    ninja-build \
    g++ \
    gcc-arm-linux-gnueabihf \
    g++-arm-linux-gnueabihf \
    qemu-user-static \
    clang-tidy \
    cppcheck \
    lcov \
    doxygen \
    python3-pip \
    python3-venv \
    git \
    && rm -rf /var/lib/apt/lists/*

RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"
RUN pip install lizard

WORKDIR /workspace
COPY . .
ENV CMAKE_BUILD_PARALLEL_LEVEL=4
