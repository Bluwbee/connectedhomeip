
FROM ubuntu:latest

#Get sudo
RUN apt-get update && apt-get install -y sudo

# Create a non-root user and add to the sudoers list
RUN adduser --disabled-password --gecos '' docker
RUN adduser docker sudo
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

USER docker

RUN sudo apt-get update \ 
    && sudo apt-get install -y gn git gcc g++ python3 pkg-config libssl-dev libdbus-1-dev libglib2.0-dev ninja-build python3-venv python3-dev unzip openssl libavahi-client-dev python3-pip libgirepository1.0-dev libcairo2-dev libreadline-dev libsdl2-dev npm wget

# Reinstall the troublesome module + install missing
RUN python3 -m pip uninstall prompt_toolkit && python3 -m pip install prompt_toolkit click lark jinja2 stringcase


