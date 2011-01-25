#ifndef REBINHISTOGRAM_TEST_H_
#define REBINHISTOGRAM_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vector>
#include "MantidKernel/VectorHelper.h"


/// @author Laurent C Chapon, ISIS Facility, Rutherford Appleton Laboratory
/// 13/03/2009
/// This is testing the validity of the rebinHistogram function in vectorHelper Class

class RebinHistogramTest : public CxxTest::TestSuite
{
public:
  /// Create a new X vector where the steps are half the size of the old one.
  /// Perform rebin and check the values
  /// Y data should now contains half the intensity
  /// E data should contains the
  /// Perform another rebin in the opposite direction and check that the data are identical to initial values
  void TestRebinSmallerSteps()
  {
    // Size of vectors
    int size1=12, size2=23;
    std::vector<double> xin(size1);
    std::vector<double> yin(size1-1);
    std::vector<double> ein(size1-1);
    std::vector<double> xout(size2);
    std::vector<double> yout(size2-1);
    std::vector<double> eout(size2-1);
    for (std::size_t i=0;i<size1-1;i++)
    {
      xin[i]=(double)(i);
      yin[i]=1.0;
      ein[i]=1.0;
    }
    xin[size1-1]=size1-1;
    for (std::size_t i=0;i<size2;i++)
      xout[i]=0.5*i;
    Mantid::Kernel::VectorHelper::rebinHistogram(xin,yin,ein,xout,yout,eout,false);
    for (std::size_t i=0;i<size2-1;i++)
    {
      TS_ASSERT_DELTA(yout[i],0.5,1e-7);
      TS_ASSERT_DELTA(eout[i],1.0/sqrt(2.0),1e-7);
    }
    std::vector<double> returnX(xin), returnY(size1-1), returnE(size1-1);

    Mantid::Kernel::VectorHelper::rebinHistogram(xout,yout,eout,returnX,returnY,returnE,false);
    for (std::size_t i=0;i<size1-1;i++)
    {
      TS_ASSERT_DELTA(returnY[i],yin[i],1e-7);
      TS_ASSERT_DELTA(returnE[i],ein[i],1e-7);
    }
  }

};


#endif //REBINHISTOGRAM_TEST_H_/
