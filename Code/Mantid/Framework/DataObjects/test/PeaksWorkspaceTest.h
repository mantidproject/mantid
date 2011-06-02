#ifndef MANTID_DATAOBJECTS_PEAKSWORKSPACETEST_H_
#define MANTID_DATAOBJECTS_PEAKSWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdio.h>
#include <cmath>
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class PeaksWorkspaceTest : public CxxTest::TestSuite
{
public:


//  int removeFile( std::string outfile)
//  {
//     return remove( outfile.c_str());
//  }
//
//  bool sameFileContents( std::string file1, std::string file2)
//  {
//    std::ifstream in1( file1.c_str() );
//    std::ifstream in2( file2.c_str() );
//
//    std::string s1, s2;
//    while (in1.good() && in2.good())
//    {
//      std::getline(in1,s1);
//      std::getline(in2,s2);
//      s1 = Strings::replace(s1, "\r", "");
//      s2 = Strings::replace(s2, "\r", "");
//      if (s1 != s2)
//        return false;
//    }
//    if( in1.good() || in2.good())
//      return false;
//    return true;
//  }


  /** Build a test PeaksWorkspace
   *
   * @return PeaksWorkspace
   */
  PeaksWorkspace * buildPW()
  {
    IInstrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, 10);
    PeaksWorkspace * pw = new PeaksWorkspace();
    Peak p(inst, 1, 3.0);
    pw->addPeak(p);
    return pw;
  }

  /** Check that the PeaksWorkspace build by buildPW() is correct */
  void checkPW(PeaksWorkspace * pw)
  {
    TS_ASSERT_EQUALS( pw->columnCount(), 17);
    TS_ASSERT_EQUALS( pw->rowCount(), 1);
    TS_ASSERT_EQUALS( pw->getNumberPeaks(), 1);
    if (pw->getNumberPeaks() != 1) return;
    TS_ASSERT_DELTA( pw->getPeak(0).getWavelength(), 3.0, 1e-4);
  }

  void test_defaultConstructor()
  {
    PeaksWorkspace * pw = buildPW();
    checkPW(pw);
    delete pw;
  }

  void test_copyConstructor()
  {
    PeaksWorkspace * pw = buildPW();
    PeaksWorkspace * pw2 = new PeaksWorkspace(*pw);
    checkPW(pw2);
    delete pw;
    delete pw2;
  }

  void test_clone()
  {
    PeaksWorkspace_sptr pw(buildPW());
    PeaksWorkspace_sptr pw2 = pw->clone();
    checkPW(pw2.get());
  }


};


#endif /* MANTID_DATAOBJECTS_PEAKSWORKSPACETEST_H_ */

