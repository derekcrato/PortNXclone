@echo off
wsl.exe -d Ubuntu -- bash -c "sudo apt update && sudo apt install -y wget gnupg2"
pause