#!/bin/sh
pwd
rm ../../../../logs/qtiplot.log
rm ../../../../logs/qtiplotErr.log
cd MantidQt
qmake-qt4
make
cd ..
cd qtiplot/
qmake-qt4 
make >> ../../../../../logs/qtiplot.log 2> ../../../../../logs/qtiplotErr.log

