
mailio
======

mailio is a cross platform C++ library for MIME format and SMTP, POP3 and IMAP protocols. It is based on standard C++ 11 and Boost library.

# Requirements #

mailio is currently under development on Linux, but it should work on any Unix derivative with gcc compiler. The following components are
required:

* gcc 4.8.2 or later.
* Boost 1.54 or later with Regex, Date Time, Random available.
* POSIX Threads, OpenSSL and Crypto libraries available on the system.

# Setup #

Since there is no auto configure tool, `Makefile` must be edited to setup Boost path and compiler. Open the favourite editor and modify
`BOOST_DIR` variable to contain the path to Boost library installation. If it is already installed, check if can be found in
`/usr` directory. For the compiler path, set the `CXX` variable to has the full path to gcc compiler. Run `gmake all`. The resulting
static and shared library should be in newly created `lib` directory.

Example are stored in `examples` directory. Run `gmake examples` to compile them. They will be placed in the same `examples` directory.

Documentation can be generated if [Doxygen](http://www.doxygen.org) is installed. By executing `gmake doc` it will be generated in `doc`
directory.

# Contact #

The library is still under heavy development. In case you find a bug, please drop me a mail to mailio (at) alepho.com. Since this is my
aside project, I'll do my best to be responsive.

# Roadmap #

The following features are planned:

* version 0.8.0: More test scenarios of using protocols with commands combined; expected timeframe is Q3-Q4/2016.
* version 0.9.0: Q codec to support non-ASCII message subjects; expected timeframe is Q4/2016.
* version 0.10.0: Test Clang and FreeBSD compatibility; expected timeframe is Q4/2016.
