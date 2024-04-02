
FROM ubuntu:latest

ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Create a non-root user
RUN groupadd -g $USER_GID $USERNAME \
    && useradd -u $USER_UID -r -g $USER_GID -s /bin/bash $USERNAME

USER root

RUN apt-get update \ 
    && apt-get install -y gn git gcc g++ python3 pkg-config libssl-dev libdbus-1-dev libglib2.0-dev ninja-build python3-venv python3-dev unzip openssl libavahi-client-dev python3-pip libgirepository1.0-dev libcairo2-dev libreadline-dev libsdl2-dev npm wget

# Reinstall the troublesome module + install missing
RUN python3 -m pip uninstall prompt_toolkit && python3 -m pip install prompt_toolkit click lark jinja2 stringcase

USER $USERNAME


