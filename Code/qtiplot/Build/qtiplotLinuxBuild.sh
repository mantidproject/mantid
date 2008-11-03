#!/bin/sh
pwd
rm ../../../../logs/qtiplot.log
rm ../../../../logs/qtiplotErr.log
cd qtiplot/
qmake-qt4 
make >> ../../../../../logs/qtiplot.log 2> ../../../../../logs/qtiplotErr.log

