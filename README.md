# apt-transport-tftp

This repository allow to download APT updates via the TFTP protocol.

This package contains the APT TFTP transport. It makes it possible to
use 'deb tftp://localhost:69/foo distro main' type lines in your
sources.list file.

# Installation

To install this package, either:

- Build this repository and put the `apt-transport-tftp` binary to `/usr/lib/apt/methods/tftp` file.
- Install the pre-built deb package from the [releases](https://github.com/amurzeau/apt-transport-tftp/releases) page.

# Building from sources

First install these dependencies:

- `cmake`
- `make`
- `g++`
- `libapt-pkg-dev`

Build commands:
```sh
# Build
mkdir build
cmake -S . -B build
cmake --build build --target package --config RelWithDebInfo

# Install at /usr/lib/apt/methods/tftp
sudo make -C build install
```
