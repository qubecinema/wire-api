
cmake_minimum_required (VERSION 3.2)

project (KeySmithClient)

find_package(Boost 1.59 REQUIRED system filesystem thread)

find_package(OpenSsl 1.0.2 REQUIRED)

if (NOT CPP-NETLIB_INCLUDE_DIR)
	message(FATAL_ERROR "CPP-NETLIB_INCLUDE_DIR not set")
endif()

if (NOT CPP-NETLIB_LIBRARY_DIR)
	message(FATAL_ERROR "CPP-NETLIB_LIBRARY_DIR not set")
endif()

# Add version number
set (KeySmithClient_VERSION_MAJOR 1)
set (KeySmithClient_VERSION_MINOR 0)

# Boost includes
include_directories("${Boost_INCLUDE_DIR}")

# Boost libraries
link_directories("${Boost_LIBRARY_DIRS}")

# Cpp-Netlib includes
include_directories("${CPP-NETLIB_INCLUDE_DIR}")

# Cpp-NetLib libraries
link_directories("${CPP-NETLIB_LIBRARY_DIR}")

# OpenSsl includes
include_directories("${OPENSSL_INCLUDE_DIR}")

SET(KeySmithClientExe ${CMAKE_SOURCE_DIR}/src/main.cpp ${CMAKE_SOURCE_DIR}/src/KeySmithClient.cpp)

ADD_EXECUTABLE(KeySmithClient ${KeySmithClientExe})

TARGET_LINK_LIBRARIES(KeySmithClient ${Boost_LIBRARIES} cppnetlib-client-connections cppnetlib-server-parsers cppnetlib-uri ${OPENSSL_LIBRARIES})

add_definitions(-DBOOST_NETWORK_ENABLE_HTTPS)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()