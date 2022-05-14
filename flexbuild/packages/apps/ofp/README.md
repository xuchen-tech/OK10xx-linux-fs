Open IP Fast Path general info
===============================================================================


Intent and purpose:
-------------------------------------------------------------------------------
The intent of this project is to enable accelerated routing/forwarding for
IPv4 and IPv6, tunneling and termination for a variety of protocols.
Unsupported functionality is provided by the host OS networking stack;
aka slowpath.

The implemented IP fastpath stack functionality is provided as a library to
Fast Path applications that use ODP execution model and framework. See an
example application: `example/fpm/app_main.c`

Termination of protocols with POSIX interface for legacy applications is also
supported.


Architecture:
-------------------------------------------------------------------------------
State is kept by the slowpath stack, i.e. routing tables, arp tables are only
written to by the slowpath side. All control and CLI functionality interacts
with the slowpath stack. Fastpath receives state notifications and information
through NETLINK API.

An end goal could be to have a full ODP IP fastpath stack that can work
independently of a slowpath in a Bare Metal environment.

Classification API should be used to further improve the fastpath performance.

Any crypto, checksum or other operation will be offloaded by ODP API with HW
support.


Coding Style:
-------------------------------------------------------------------------------
Project code uses Linux kernel style that is verified through `checkpatch.pl`


Licensing:
-------------------------------------------------------------------------------
Project uses BSD 3-CLause License as default license. One should not use code
that is licensed under any GPL type.


Mailing list
-------------------------------------------------------------------------------
We have a mailing list, reached via the address: _openfastpath@list.openfastpath.org_


Open IP Fast Path getting started
===============================================================================


Build environment preparation:
-------------------------------------------------------------------------------
This project is currently verified on a generic 32/64bit x86 Linux machine.

The following packages are mandatory for accessing and building ODP and OFP:

    git aclocal libtool automake build-essential pkg-config

The following packages are optional:

    libssl-dev doxygen asciidoc valgrind libcunit1 libcunit1-doc libcunit1-dev

Download ODP:

    git clone https://git.linaro.org/lng/odp.git

Enter the odp directory, prepare for build and build:

    ./bootstrap
    ./configure \
      --prefix=<INSTALL ODP TO THIS DIR>
    make
    make install
(Note; `make install` may require root permissions)


Build environment for ThunderX ODP:
-------------------------------------------------------------------------------
ThunderX ODP platform sould be cross-compiled or compiled natively with
Cavium-provided toolchain: `aarch64-thunderx-linux-gnu-gcc`

The build procedure is a standard procedure for ODP platfrom with
following notes:

  - Certain OpenSSL versions may require `-ldl` library for linking
against dlopen()
  - The best CFLAGS for performance are `-O3 -static -flto`

Below are few examples:

Typical compilation with shared library and standard CFLAGS (-O2):

    ./bootstrap
    ./configure --host=aarch64-thunderx-linux-gnu \
      --with-platform=linux-thunder \
      --with-openssl-path=${OPENSSL_DIR} \
      --disable-debug-print \
      --disable-debug \
      --prefix=${ODP_DIR} \
      LIBS="-ldl"
    make install

Recommended, performance-optimized build (static library and aggresive
optimizations):

    ./bootstrap
    ./configure --host=aarch64-thunderx-linux-gnu \
      --with-platform=linux-thunder \
      --with-openssl-path=${OPENSSL_DIR} \
      --disable-debug-print \
      --disable-debug \
      --prefix=${ODP_DIR} \
    CFLAGS="-O3 -static -flto -g" LIBS="-ldl"
    make install

Debugging build:

    ./bootstrap
    ./configure --host=aarch64-thunderx-linux-gnu \
      --with-platform=linux-thunder \
      --with-openssl-path=${OPENSSL_DIR} \
      --enable-debug-print \
      --enable-debug \
      --prefix=${ODP_DIR} \
    CFLAGS="-O0 -static -g" LIBS="-ldl"
    make install


Access OFP source code and build
-------------------------------------------------------------------------------
    git clone https://github.com/OpenFastPath/ofp

Enter the OFP directory, prepare for build and build:

    ./bootstrap
    ./configure \
      --with-odp=<ODP INSTALLATION DIR> \
      --enable-cunit

        (if using DPDK)
      --enable-shared=no
      --enable-sp=no
    make
(Note; "make check" can be issued to verify the OFP build)


File structure definition
-------------------------------------------------------------------------------
./docs/        - This is where you can find more detailed documentation<br>
./example/fpm/ - Template application example that uses the project library<br>
./include/api/ - Public interface headers used by an application.<br>
./include/     - Internal interface headers that are used in fastpath library.<br>
./scripts/     - Scripts that start/stop the application and configure system.<br>
./src/         - .c files with fastpath library implementation.<br>
./src/cli/     - Command Line Interface implementation used mainly for debug.<br>
./test/cunit/  - CUnit testcases implementation


Execute OFP example applications:
-------------------------------------------------------------------------------
A `start_device.sh/stop_device.sh` script is available in the scripts directory
to start/stop the fpm example application. By default the `eth0` interface is
used for the fastpath processing but any other interface/interfaces names can
be supplied to the script as parameter.

Example usage of generic non-accelerated build in a linux environment:

    <OFPROOT>/scripts/start_device.sh eth0

This will create a `fp0` interface that can be seen when issuing `ifconfig -a`

    <OFPROOT>/example/fpm/fpm -i fp0

This will start the fpm example module using the `fp0` interface, the CLI
can be accessed if issuing `telnet 127.0.0.1 2345`

When using `ethX` interface for fastpath processing this will be disconnected
from Linux. A `fpY` TUN/TAP interface is created by fastpath application and
this can be used by Linux to send and receive packets.
The sent packets from Linux on `fpY` interface are forwarded to `ethX` (wire).
The packets received by the `ethX` interface are captured by ODP and then these
are received by FPM fastpath application.
When no fastpath operation is aplicable the packet is forwarded to slowpath.


Tools
===============================================================================


Code coverage:
-------------------------------------------------------------------------------
Generate code coverage report from unit tests by passing `--coverage` during
building, and use `lcov` to view the results:

    ./configure <typical-ofp-flags> CFLAGS='-g -O0 --coverage' \
      LDFLAGS='--coverage'
    make check

    cd test/cunit

    lcov --directory . --directory ../../src --capture --output-file \
      coverage.info

    genhtml coverage.info --output-directory out

view `out/index.html`

