# Sqwawk

A compact XMPP desktop messenger

### Prerequisites

- QT 5.12 *(lower versions might work but it wasn't tested)*
- uuid _(usually included in some other package, for example it's ***libutil-linux*** in archlinux)_
- lmdb
- CMake 3.0 or higher

### Building

Squawk requires Qt with SSL enabled. It uses CMake as build system.

Squawk uses upstream version of QXmpp library so first we need to pull it
```
git submodule update --init --recursive
```
Then create a folder for the build, go there and build the project using CMake
 
```
mkdir build
cd build
cmake ..
cmake --build .
```

## License

This project is licensed under the GPLv3 License - see the [LICENSE.md](LICENSE.md) file for details
