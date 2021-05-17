#! /bin/bash
#

mkdir build
cd build
cmake -D BOOST_ROOT=../boost -D OPENSSL_ROOT_DIR=../openssl  ..
