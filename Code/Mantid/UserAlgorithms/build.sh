#!/bin/sh
# 
# Builds the example algorithms shared library for those not using scons
#
g++ -fPIC -c -I ../API/inc -I ../Kernel/inc -I ../Geometry/inc -I ../DataObjects/inc -I ../../Third_Party/include *.cpp
g++ -shared -o libUsersAlgorithms.so *.o
mv libUsersAlgorithms.so ../Bin/Plugins/
rm -f *.o
