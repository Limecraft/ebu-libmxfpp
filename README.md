# libMXF++

libMXF++ is a C++ wrapper library for libMXF that supports reading and writing the [SMPTE ST 377-1 MXF file format](https://ieeexplore.ieee.org/document/7292073).

libMXF++ and libMXF are used in the bmx project.

libMXF++ was originally developed as part of the
[Ingex Project](http://ingex.sourceforge.net/) where it supported MXF transfer,
playback and storage applications.


## Examples

A number of examples can be found in the
[examples](./examples) directory. These are not part of the core library and
are not required to build bmx.
* [D10MXFOP1AWriter](./examples/D10MXFOP1AWriter): library code used in the
[Ingex Project](http://ingex.sourceforge.net/) for writing [SMPTE ST
386](https://ieeexplore.ieee.org/document/7291350) MXF D-10 (Sony MPEG IMX)
files.
* [OPAtomReader](./examples/OPAtomReader): library code used in the
[Ingex Project](http://ingex.sourceforge.net/) for recovering Avid MXF OP-Atom
files after a system failure.


## Build and Installation

libMXF++ is developed on Ubuntu Linux but is supported on other Unix-like
systems using the autotools build system. A set of Microsoft Visual C++ project
files are provided for Windows.


### Dependencies

The following libraries must be installed to build libMXF++. The (Ubuntu) debian
package names and versions are shown in brackets.
* libMXF (libmxf >= 1.0.3)


### Unix-like Systems Build

Install the development versions of the dependency libraries. The libMXF++
library and example applications can then be built from source using autotools
as follows,
```bash
./autogen.sh
./configure
make
```

Run
```bash
./configure -h
```
to see a list of build configuration options.

A number of `--disable-*` options are provided for disabling all examples
(`--disable-examples`) or specific ones (e.g.
`--disable-opatom-reader`). The `--disable-examples` option can be combined with
`--enable-*` options to enable specific examples. The
bmx project does not require the examples and therefore libMXF++ can be
configured using `--disable-examples`.

If you get library link errors similar to "error while loading shared
libraries" then run
```
sudo /sbin/ldconfig
```
to update the runtime linker cache. E.g. the libMXF library was built and
installed previously and the linker cache needs to be updated with the result.

There are a number of core library and example regression tests that can be run
using
```bash
make check
```

Finally, the core library and examples can be installed using
```bash
sudo make install
```

To avoid library link errors similar to "error while loading shared
libraries" when building bmx run
```bash
sudo /sbin/ldconfig
```
to update the runtime linker cache after installation.


### Microsoft Visual Studio C++ Build

The Visual Studio 2010 build solution and project files can be found in the
[msvc_build/vs10](./msvc_build/vs10) directory. These files can be upgraded to
any more recent version when importing into the IDE.

The build solution file is [libMXF++.sln](./msvc_build/vs10/libMXF++.sln).
It is used to build the library and D10MXFOP1AWriter example. The build solution
assumes that the `libMXF/` project is present at the same directory level as
libMXF++. The build solution file will build the libMXF library.

The build depends on the `mxfpp_scm_version.h` header file in the root directory
to provide the most recent git commit identifier. This file is generated
automatically using the [gen_scm_version.sh](./gen_scm_version.sh) script when
building using autotools and is included in the source distribution package.
You are likely missing this file if you are using the source code directly from
the git repository and will need to create it manually.


## Source and Binary Distributions

Source distributions and Windows binaries are made [available on
SourceForge](https://sourceforge.net/projects/bmxlib/files/).


## License

The libMXF++ library is provided under the BSD 3-clause license. See the
[COPYING](./COPYING) file provided with this library for more details.
