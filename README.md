# UntitledDBusUtils

[![CI](https://github.com/MadLadSquad/UntitledDBusUtils/actions/workflows/CI.yaml/badge.svg)](https://github.com/MadLadSquad/UntitledDBusUtils/actions/workflows/CI.yaml)
[![MIT license](https://img.shields.io/badge/License-MIT-blue.svg)](https://lbesson.mit-license.org/)
[![trello](https://img.shields.io/badge/Trello-UDE-blue])](https://trello.com/b/HmfuRY2K/untitleddesktop)
[![Discord](https://img.shields.io/discord/717037253292982315.svg?label=&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2)](https://discord.gg/4wgH8ZE)

C++ Utilities for dealing with the low level C DBus API. Features:
1. RAII classes for the following constructs:
   - Errors
   - Messages
   - Connections
   - Pending calls
1. Heavy usage of C++ features to provide the following:
   - Automatic type recognition when appending method arguments
   - Type safety and automatic termination of method append calls
   - <2 line serialisation and de-serialisation of complex components such as structs and maps
1. Additional pointer checks to prevent segmentation faults that are common when using dbus-1

## Learning
All documentation can be found on the [wiki](https://github.com/MadLadSquad/UntitledDBusUtils/wiki/).
