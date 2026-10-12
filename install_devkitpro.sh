#!/bin/bash
set -e
sudo apt update && sudo apt install -y wget gnupg2
wget -q https://github.com/devkitPro/pacman/releases/download/v1.0.2/devkitpro-pacman.amd64.deb
sudo dpkg -i devkitpro-pacman.amd64.deb
echo 'export DEVKITPRO=/opt/devkitpro' >> ~/.bashrc
echo 'export DEVKITARM=/opt/devkitpro/devkitARM' >> ~/.bashrc
echo 'export PATH=$PATH:$DEVKITPRO/tools/bin' >> ~/.bashrc
source ~/.bashrc
sudo dkp-pacman -Sy --noconfirm switch-dev switch-portlibs