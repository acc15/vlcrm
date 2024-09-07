# vlcrm

Simple VLC plugin which allows to delete currently playing file using hotkey.

## Prebuilt binaries

* MacOS (ARM64, Apple Silicon): https://github.com/acc15/vlcrm/releases/download/v1.0/libvlcrm_plugin.dylib

You must place it to VLC plugins directory:

* Windows: `C:\Program Files\VideoLAN\VLC\plugins\misc`
* MacOS: `/Applications/VLC.app/Contents/MacOS/plugins`
* Linux: `/usr/lib/vlc/plugins/misc`

Then follow [Running](#running) guide

## Building

### Prerequisites

* VLC 3.0.20+ must be installed. Previous versions may work, but not tested
* CMake 3.29.3
* Working C compiler (`MSVC`, `GCC` or `Clang`) with C 17 support

Compiling requires `libvlccore` headers to be available.

On some systems (for example on ArchLinux) they are already in `vlc` package 
and located in `/usr/include/vlc/plugins/` directory.

On MacOS and Windows VLC installation they are absent, so it's required to download VLC sources to get those headers.

CMake will automatically download them using `FetchContent` feature if they are not available.

### Compile

    cmake -DCMAKE_BUILD_TYPE=Release -B build && cmake --build build

#### Additional configuration

By default CMake will assume default VLC install locations:

* Windows: `C:\Program Files\VideoLAN\VLC`
* MacOS: `/Applications/VLC.app/Contents/MacOS`
* Linux: `/usr/lib`

and also it will download VLC sources if they aren't found in `/usr/include/vlc/plugins/` directory. 

There are 3 CMake variables which allows to customize where to search for `libvlccore` headers, library and where to put builded plugin:

| Name          | Description                            |
| ------------- | -------------------------------------- |
| `VLC_HEADERS` | Path to `libvlccore` headers directory |
| `VLC_LIB`     | Path to `libvlccore` library directory |
| `VLC_PLUGINS` | Path to plugin installation directory  |

To specify your own custom directories you can run `cmake` as follows:

    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DVLC_HEADERS=/home/user/vlc_sources/include \
        -DVLC_LIB=/home/user/myvlc/lib \
        -DVLC_PLUGINS=/home/user/myvlc/lib/plugins/misc \
        -B build && cmake --build build

### Install

    cmake --install build

## Running

After installation plugin must be explicitly enabled using VLC settings interface:
![enable_vlcrm](images/enable_vlcrm.png)
Restart VLC, and plugin should activate, you can check it's settings page:
![vlcrm_settings](images/vlcrm_settings.png)
Then just use corresponding key combination.