Overview
========
QubeWireClient is a C++ application demonstrating Qube Wire mastering APIs. It demonstrates the following functionalities of a mastering client.
    - Logging in and acquiring access token
    - Obtaining logged in user and company information
    - Signing a CPL/PKL using Qube Wire
    - Uploading DKDM into Qube Wire

Dependencies
============
The following C++ libraries are required to build QubeWireClient.
    - Boost v1.58.0 or latest
    - cpp-netlib v0.11.1 or latest
    - OpenSSL 1.0.2 or latest

Build Instructions
=================
Extract the source of QubeWireClient into a directory and execute the following inside the directory.

Linux and Mac OSX:
    $ mkdir build
    $ cd build
    $ cmake -DCPP-NETLIB_INCLUDE_DIR=<cpp-netLib include dir path> -DCPP-NETLIB_LIBRARY_DIR=<cpp-NetLib library dir path> ..
    $ make

    The above cmake command assumes OpenSSL and Boost are installed in system default directories. Refer the below Windows build instructions to use OpenSSL/Boost from non-system default directores.

Windows:
    $ mkdir build
    $ cd build
    $ set OPENSSL_ROOT_DIR=<openssl dir path>
    $ cmake -DBOOST_INCLUDEDIR=<boost include dir path> -DBOOST_LIBRARYDIR=<boost library dir path> -DCPP-NETLIB_INCLUDE_DIR=<cpp-netLib include dir path> -DCPP-NETLIB_LIBRARY_DIR=<cpp-netLib library dir path> ..
    $ make
