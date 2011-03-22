#ifndef BINFINDERTEST_H_
#define BINFINDERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/BinFinder.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VectorHelper.h"
#include <sys/stat.h>

using namespace Mantid;
using namespace Mantid::Kernel;

using std::runtime_error;
using std::size_t;
using std::vector;
using std::cout;
using std::endl;

//==========================================================================================
class BinFinderTest: public CxxTest::TestSuite
{
public:
  void testLinearBins()
  {
    std::vector<double> bp;
    //0 to 100 in steps of 2.
    bp.push_back(0.0);
    bp.push_back(2.0);
    bp.push_back(100.0);
    BinFinder bf(bp);
    //Try a few indices
    TS_ASSERT_EQUALS( bf.bin(-0.1), -1);
    TS_ASSERT_EQUALS( bf.bin(100.2), -1);
    TS_ASSERT_EQUALS( bf.bin(0.0), 0);
    TS_ASSERT_EQUALS( bf.bin(0.1), 0);
    TS_ASSERT_EQUALS( bf.bin(1.999), 0);
    TS_ASSERT_EQUALS( bf.bin(2.0), 1);
    TS_ASSERT_EQUALS( bf.bin(99), 49);
    TS_ASSERT_EQUALS( bf.lastBinIndex(), 50);
  }

  void testLogBins()
  {
    std::vector<double> bp;
    //2 to 1024, multiplying by 2 at each bin
    bp.push_back(2.0);
    bp.push_back(-1.0);
    bp.push_back(1024.0);
    BinFinder bf(bp);
    //Try a few indices
    TS_ASSERT_EQUALS( bf.bin(1.8), -1);
    TS_ASSERT_EQUALS( bf.bin(1025.0), -1);
    TS_ASSERT_EQUALS( bf.bin(2.0), 0);
    TS_ASSERT_EQUALS( bf.bin(2.1), 0);
    TS_ASSERT_EQUALS( bf.bin(3.999), 0);
    TS_ASSERT_EQUALS( bf.bin(4.0), 1);
    TS_ASSERT_EQUALS( bf.bin(6.0), 1);
    TS_ASSERT_EQUALS( bf.bin(8.1), 2);
    TS_ASSERT_EQUALS( bf.bin(16.1), 3);
    TS_ASSERT_EQUALS( bf.bin(32.1), 4);
    TS_ASSERT_EQUALS( bf.bin(64.1), 5);
    TS_ASSERT_EQUALS( bf.bin(128.1), 6);
    TS_ASSERT_EQUALS( bf.bin(256.1), 7);
    TS_ASSERT_EQUALS( bf.bin(512.1), 8);
    TS_ASSERT_EQUALS( bf.bin(1023.9), 8);
    TS_ASSERT_EQUALS( bf.lastBinIndex(), 9);
  }

  void testCompoundBins()
  {
    //3 binning regions
    std::vector<double> bp;
    bp.push_back(-10.0);
    bp.push_back(10.0);
    bp.push_back(102.0);
    bp.push_back(100.0); //now step in 100
    bp.push_back(1000.0);
    bp.push_back(1000.0); //now step in 100
    bp.push_back(10000.0);
    BinFinder bf(bp);
    //Try a few indices
    TS_ASSERT_EQUALS( bf.bin(-11), -1);
    TS_ASSERT_EQUALS( bf.bin(10000.2), -1);
    TS_ASSERT_EQUALS( bf.bin(-5.0), 0);
    TS_ASSERT_EQUALS( bf.bin(5), 1);
    TS_ASSERT_EQUALS( bf.bin(15), 2);
    TS_ASSERT_EQUALS( bf.bin(95), 10);
    TS_ASSERT_EQUALS( bf.bin(101), 10); //Last tiny bin of size 2 is folded into the one before
    TS_ASSERT_EQUALS( bf.bin(105), 11);
    TS_ASSERT_EQUALS( bf.bin(195), 11);
    TS_ASSERT_EQUALS( bf.bin(205), 12);
    TS_ASSERT_EQUALS( bf.bin(995), 19);
    TS_ASSERT_EQUALS( bf.bin(1005), 20);
    TS_ASSERT_EQUALS( bf.bin(2005), 21);
    TS_ASSERT_EQUALS( bf.lastBinIndex(), 29);
  }

  void testCompoundBinsWithLog()
  {
    std::vector<double> bp;
    bp.push_back(2.0);
    bp.push_back(-1.0);
    bp.push_back(1100.0);
    bp.push_back(100.0); //now linear
    bp.push_back(2000.0);
    BinFinder bf(bp);
    //Try a few indices
    TS_ASSERT_EQUALS( bf.bin(1.8), -1);
    TS_ASSERT_EQUALS( bf.bin(2001), -1);
    TS_ASSERT_EQUALS( bf.bin(2.1), 0);
    TS_ASSERT_EQUALS( bf.bin(512.1), 8);
    TS_ASSERT_EQUALS( bf.bin(1025), 8); //The last log bin is too small, folded into bin 8
    TS_ASSERT_EQUALS( bf.bin(1101), 9);
    TS_ASSERT_EQUALS( bf.bin(1201), 10);
    TS_ASSERT_EQUALS( bf.lastBinIndex(), 18);
  }

  /// Compare the # of bins that the BinFinder computes to the # found by the vector helper.
  void compareBin(double x1, double step, double x2)
  {
    std::vector<double> bp;
    bp.push_back(x1);
    bp.push_back(step);
    bp.push_back(x2);
    BinFinder bf(bp);
    std::vector<double> X;
    VectorHelper::createAxisFromRebinParams(bp, X);
    TS_ASSERT_EQUALS( bf.lastBinIndex()+1, X.size());
  }

  void testNumberOfBinMatchesVectorHelper()
  {
    compareBin(1., 1, 2.1);
    compareBin(-2., 100, 2);
    compareBin(1.0, -1, 16.);
    compareBin(1.0, -1, 16.1);
  }

};

#endif /*BINFINDERTEST_H_*/


