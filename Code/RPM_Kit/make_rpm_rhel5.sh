#!/bin/sh
env CC="gcc44" CXX="g++44" MANTID_BUILD_FLAGS="gcc44=1" sh make_rpm.sh $*

