# Тулчейн-образ ЛР2: прошивка (arm-none-eabi-gcc), хостові тести (gcc + lcov),
# інструменти (python3) і декодування записів аналізатора (sigrok-cli).
#
# Відтворюваність: базовий образ — фіксований реліз Ubuntu 22.04 (jammy), пакети з
# його архіву (gcc-arm-none-eabi 10.3-2021.10, gcc 11, lcov 1.15, sigrok-cli 0.7.2).
# Точний стан фіксується `make versions` у CI-лозі. Для побітової відтворюваності
# через рік замініть тег на digest: docker pull ubuntu:22.04 &&
# docker inspect --format='{{index .RepoDigests 0}}' ubuntu:22.04  → FROM ubuntu@sha256:...
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
      make gcc-arm-none-eabi binutils-arm-none-eabi \
      libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib \
      gcc libc6-dev lcov \
      python3 git ca-certificates srecord \
      sigrok-cli \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /workspace
