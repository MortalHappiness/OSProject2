# OSProject2
Project 2 for the course "Operating System" in NTU.

## Project Spec.
https://hackmd.io/@buB4b8JxReG7RX6wHM9Iwg/HkeM7wt58

## Website
https://mortalhappiness.github.io/OSProject2/

## Execution
1. Change to super user
```sh
su -
```
2. Execute `compile.sh` to compile codes and install modules.
```sh
bash compile.sh
```
3. Execute the master and slave programs.
```sh
./master 1 file1_in mmap
./slave 1 file1_out fcntl 127.0.0.1
```
Make sure that you are under the path "./user_program" when you execute user programs.
Though the execution order of user program "master" and "slave" does not matter,
it is suggested to execute "master" first to get more precise transmission time.

## Directory structure

    .
    ├── miniex            - Supplemental information for this project, provided by TA.
    ├── ksocket           - The device moudule including the funtions used for kernel socket.
    ├── master_device     - The device moudule for master server.
    ├── slave_device      - The device moudule for slave client.
    ├── user_program      - The user program "master" and "slave".
    ├── input
    ├── output
    └── demo

