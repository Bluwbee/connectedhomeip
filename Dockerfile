
FROM ubuntu:latest


RUN apt-get update \ 
    && apt-get install -y gn git gcc g++ python3 pkg-config libssl-dev libdbus-1-dev libglib2.0-dev ninja-build python3-venv python3-dev unzip openssl libavahi-client-dev python3-pip libgirepository1.0-dev libcairo2-dev libreadline-dev libsdl2-dev npm wget libpixman-1-dev libcairo-dev libsdl-pango-dev libjpeg-dev libgif-dev

# Reinstall the troublesome module + install missing
RUN python3 -m pip uninstall prompt_toolkit && python3 -m pip install prompt_toolkit click lark jinja2 stringcase


