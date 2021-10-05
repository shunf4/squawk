# Squawk - a compact XMPP desktop messenger

[![AUR license](https://img.shields.io/aur/license/squawk?style=flat-square)](https://git.macaw.me/blue/squawk/raw/branch/master/LICENSE.md)
[![AUR version](https://img.shields.io/aur/version/squawk?style=flat-square)](https://aur.archlinux.org/packages/squawk/)
[![Liberapay patrons](https://img.shields.io/liberapay/patrons/macaw.me?logo=liberapay&style=flat-square)](https://liberapay.com/macaw.me)

![Squawk screenshot](https://macaw.me/images/squawk/0.1.4.png)

### Prerequisites

- QT 5.12 *(lower versions might work but it wasn't tested)*
- lmdb
- CMake 3.0 or higher
- qxmpp 1.1.0 or higher
- kwallet (optional)

### Getting

The easiest way to get the Squawk is to install it from AUR (if you use Archlinux like distribution)

Here is the [link](https://aur.archlinux.org/packages/squawk/) for the AUR package

You can also install it from console if you use some AUR wrapper. Here what it's going to look like with *pacaur*

```
$ pacaur -S squawk
```

### Building

You can also clone the repo and build it from source

Squawk requires Qt with SSL enabled. It uses CMake as build system.

There are two ways to build, it depends whether you have qxmpp installed in your system

#### Building with system qxmpp

Here is what you do

```
$ git clone https://git.macaw.me/blue/squawk
$ cd squawk
$ mkdir build
$ cd build
$ cmake .. [-DLMDB_DIR:PATH=/path/to/lmdb]
$ cmake --build .
```

#### Building with bundled qxmpp

Here is what you do

```
$ git clone --recurse-submodules https://git.macaw.me/blue/squawk
$ cd squawk
$ mkdir build
$ cd build
$ cmake .. -D SYSTEM_QXMPP=False [-DLMDB_DIR:PATH=/path/to/lmdb]
$ cmake --build .
```

### List of keys

Here is the list of keys you can pass to configuration phase of `cmake ..`. 
- `CMAKE_BUILD_TYPE` - `Debug` just builds showing all warnings, `Release` builds with no warnings and applies optimizations (default is `Debug`)
- `SYSTEM_QXMPP` - `True` tries to link against `qxmpp` installed in the system, `False` builds bundled `qxmpp` library (default is `True`)
- `WITH_KWALLET` - `True` builds the `KWallet` capability module if `KWallet` is installed and if not goes to `False`. `False` disables `KWallet` support (default is `True`)

## License

This project is licensed under the GPLv3 License - see the [LICENSE.md](LICENSE.md) file for details
