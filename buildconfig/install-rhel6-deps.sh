#! /bin/sh
#
# This script will install all of the dependencies required to build
# Mantid on RHEL6. Obviously, you will need sudo install rights to run it.
#

sudo yum install boost-devel
sudo yum install gsl-devel
sudo yum install poco-devel
sudo yum install numpy
sudo yum install OpenCASCADE-devel
sudo yum install google-perftools-devel
sudo yum install muParser-devel
sudo yum install nexus
sudo yum install python-devel
#required for mantidplot
sudo yum install qt4-devel
sudo yum install qwt-devel
sudo yum install qwtplot3d-qt4-devel
sudo yum install sip-devel
sudo yum install qscintilla-devel
sudo yum install PyQt4-devel

