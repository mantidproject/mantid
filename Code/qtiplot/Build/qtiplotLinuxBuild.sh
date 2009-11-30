#!/bin/sh
pwd
rm ../../../../logs/qtiplot.log
rm ../../../../logs/qtiplotErr.log
cd MantidQt
qmake-qt4
make clean
make
cd ..
cd QtPropertyBrowser
qmake-qt4
make clean
make
cd ..
cd qtiplot/
qmake-qt4
make clean
make >> ../../../../../logs/qtiplot.log 2> ../../../../../logs/qtiplotErr.log

