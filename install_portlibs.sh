#!/bin/bash
set -e
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=/opt/devkitpro/devkitARM
export PATH=$PATH:$DEVKITPRO/tools/bin
sudo dkp-pacman -Sy --noconfirm switch-dev switch-portlibs