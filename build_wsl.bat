@echo off
echo Starting WSL build...
wsl.exe -d Ubuntu -- bash -c "
  cd /mnt/c/Users/Administrador/Desktop/PortNX-master/PortNX-master
  echo 'In WSL, installing dependencies...'
  sudo apt update && sudo apt install -y make git
  echo 'Installing devkitPro...'
  # Download and install devkitPro
  wget -q https://github.com/devkitPro/pacman/releases/download/v1.0.2/devkitpro-pacman.amd64.deb
  sudo dpkg -i devkitpro-pacman.amd64.deb
  echo 'export DEVKITPRO=/opt/devkitpro' >> ~/.bashrc
  echo 'export DEVKITARM=/opt/devkitpro/devkitARM' >> ~/.bashrc
  echo 'export PATH=\$PATH:\$DEVKITPRO/tools/bin' >> ~/.bashrc
  source ~/.bashrc
  echo 'Installing switch-dev and portlibs...'
  sudo dkp-pacman -Sy --noconfirm switch-dev switch-portlibs
  echo 'Building Plutonium...'
  cd lib/plutonium/Plutonium
  make -j\$(nproc)
  echo 'Building PortNX...'
  cd /mnt/c/Users/Administrador/Desktop/PortNX-master/PortNX-master
  make -j\$(nproc)
  echo 'Build complete! PortNX.nro should be in current directory.'
"
echo Build script finished.
pause