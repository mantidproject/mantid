cxxtestgen.pl --error-printer -o TestHistogram1D.cpp TestHistogram1D.h
g++ -o TestHistogram1D TestHistogram1D.cpp ../src/Histogram1D.cpp -I'/usr/opt/cxxtest'
