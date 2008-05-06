#!/bin/sh
# 
# Builds the example algorithms shared library for those not using scons
#
g++ -fPIC -c -I ../API/inc -I ../Kernel/inc -I ../Geometry/inc HelloWorldAlgorithm.cpp
g++ -shared -o libUsersAlgorithms.so HelloWorldAlgorithm.o
mv libUsersAlgorithms.so ../Bin/Plugins/
rm -f HelloWorldAlgorithm.o
