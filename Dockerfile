
FROM ubuntu:22.04

# This Dockerfile contains things useful for an interactive development environment
ARG USERNAME=jenkins
ARG USER_UID=130
ARG USER_GID=$USER_UID
ENV LANG en_US.utf8

# these are installed for terminal/dev convenience.  If more tooling for build is required, please
#  add them to chip-build (in integrations/docker/images/chip-build)
RUN apt-get update \
    && apt-get install -y locales \
    && localedef -i en_US -c -f UTF-8 -A /usr/share/locale/locale.alias en_US.UTF-8 \
    && apt-get -fy install git vim emacs sudo \
    apt-utils dialog zsh \
    iproute2 procps lsb-release \
    bash-completion \
    build-essential cmake cppcheck valgrind \
    wget curl telnet \
    docker.io \
    iputils-ping net-tools \
    libncurses5 \
    libncursesw5 \
    libpython2.7 \
    && :

RUN apt-get update \ 
    && apt-get install -y gn git gcc g++ python3 pkg-config libssl-dev libdbus-1-dev libglib2.0-dev ninja-build python3-venv python3-dev unzip openssl libavahi-client-dev python3-pip libgirepository1.0-dev libcairo2-dev libreadline-dev libsdl2-dev npm wget libpixman-1-dev libcairo-dev libsdl-pango-dev libjpeg-dev libgif-dev

# Reinstall the troublesome module + install missing
RUN python3 -m pip uninstall prompt_toolkit && python3 -m pip install prompt_toolkit click lark jinja2 stringcase

RUN groupadd -g $USER_GID $USERNAME \
    && useradd -s /bin/bash -u $USER_UID -g $USER_GID -G docker,sudo -m $USERNAME \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME \
    && :
