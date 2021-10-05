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
- KDE Frameworks: kwallet (optional)
- KDE Frameworks: KIO (optional)
- Boost

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

Please check the prerequisites and install them before installation.

#### For Windows (Mingw-w64) build

You need Qt for mingw64 (MinGW 64-bit) platform when installing Qt.

The best way to acquire library `lmdb` and `boost` is through Msys2.

First install Msys2, and then install `mingw-w64-x86_64-lmdb` and `mingw-w64-x86_64-boost` by pacman.

Then you need to provide the cmake cache entry when calling cmake for configuration:

```
cmake .. -D LMDB_ROOT_DIR:PATH=<Msys2 Mingw64 Root Directory> -D BOOST_ROOT:PATH=<Msys2 Mingw64 Root Directory>
```

`<Msys2 Mingw64 Root Directory>`: e.g. `C:/msys64/mingw64`.

---

There are two ways to build, it depends whether you have qxmpp installed in your system

#### Building with system qxmpp

Here is what you do

```
$ git clone https://git.macaw.me/blue/squawk
$ cd squawk
$ mkdir build
$ cd build
$ cmake .. [-D LMDB_ROOT_DIR:PATH=...] [-D BOOST_ROOT:PATH=...]
$ cmake --build .
```

#### Building with bundled qxmpp

Here is what you do

```
$ git clone --recurse-submodules https://git.macaw.me/blue/squawk
$ cd squawk
$ mkdir build
$ cd build
$ cmake .. -D SYSTEM_QXMPP=False [-D LMDB_ROOT_DIR:PATH=...] [-D BOOST_ROOT:PATH=...]
$ cmake --build .
```

You can always refer to `appveyor.yml` to see how AppVeyor build squawk.

### List of keys

Here is the list of keys you can pass to configuration phase of `cmake ..`. 
- `CMAKE_BUILD_TYPE` - `Debug` just builds showing all warnings, `Release` builds with no warnings and applies optimizations (default is `Debug`)
- `SYSTEM_QXMPP` - `True` tries to link against `qxmpp` installed in the system, `False` builds bundled `qxmpp` library (default is `True`)
- `WITH_KWALLET` - `True` builds the `KWallet` capability module if `KWallet` is installed and if not goes to `False`. `False` disables `KWallet` support (default is `True`)
- `WITH_KIO` - `True` builds the `KIO` capability module if `KIO` is installed and if not goes to `False`. `False` disables `KIO` support (default is `True`)

## License

This project is licensed under the GPLv3 License - see the [LICENSE.md](LICENSE.md) file for details
