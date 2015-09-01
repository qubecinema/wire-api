Build instructions for KeySmithClient

KeySmithClient application can be build for Mac as well as windows.

-> Build instruction for Mac CLI of KeySmithClient:-
	<CMAKE_PATH_OF_KEYSMITHCLIENT> - This is the CMakeList.txt path where KeySmithClient source is extracted.

  I) Prerequisites:- 
  	1) Boost latest version is needed. We have used Boost v1.59.0.
  	2) Cpp-NetLib latest version is needed. We have used Cpp-Netlib v0.11.1-final.
  	3) OpenSsl - Default openssl from Mac will be used by Cpp-NetLib.
  
  II) Build KeySmithCLient:-
  	1) pushd <CMAKE_PATH_OF_KEYSMITHCLIENT>
  	1. mkdir build
  	2. pushd build
  	3. cmake -DBOOST_INCLUDEDIR=<Boost include path> -DBOOST_LIBRARYDIR=<boost library path> -DCPP-NETLIB_INCLUDE_DIR=<Cpp-NetLib include path> -DCPP-NETLIB_LIBRARY_DIR=<Cpp-NetLib library path> ..
  	4. make

-> Build instruction for Windows CLI of KeySmithClient:-
	
	I) TODO