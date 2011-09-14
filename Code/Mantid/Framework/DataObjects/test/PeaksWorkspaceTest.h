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
#include "MantidKernel/V3D.h"
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

  /** Build a test PeaksWorkspace
   *
   * @return PeaksWorkspace
   */
  PeaksWorkspace * buildPW()
  {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, 10);
    inst->setName("SillyInstrument");
    PeaksWorkspace * pw = new PeaksWorkspace();
    pw->setInstrument(inst);
    std::string val = "value";
    pw->mutableRun().addProperty("TestProp", val);
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
    // Experiment info stuff got copied
    TS_ASSERT_EQUALS( pw->getInstrument()->getName(), "SillyInstrument");
    TS_ASSERT( pw->run().hasProperty("TestProp") );
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

  void test_sort()
  {
    PeaksWorkspace_sptr pw(buildPW());
    Instrument_const_sptr inst = pw->getInstrument();
    Peak p0 = pw->getPeak(0); //Peak(inst, 1, 3.0)
    Peak p1(inst, 1, 4.0);
    Peak p2(inst, 1, 5.0);
    Peak p3(inst, 2, 3.0);
    Peak p4(inst, 3, 3.0);
    pw->addPeak(p1);
    pw->addPeak(p2);
    pw->addPeak(p3);
    pw->addPeak(p4);

    std::vector< std::pair<std::string, bool> > criteria;
    // Sort by detector ID then descending wavelength
    criteria.push_back( std::pair<std::string, bool>("detid", true) );
    criteria.push_back( std::pair<std::string, bool>("wavelength", false) );
    pw->sort(criteria);
    TS_ASSERT_EQUALS( pw->getPeak(0).getDetectorID(), 1);
    TS_ASSERT_DELTA(  pw->getPeak(0).getWavelength(), 5.0, 1e-5);
    TS_ASSERT_EQUALS( pw->getPeak(1).getDetectorID(), 1);
    TS_ASSERT_DELTA(  pw->getPeak(1).getWavelength(), 4.0, 1e-5);
    TS_ASSERT_EQUALS( pw->getPeak(2).getDetectorID(), 1);
    TS_ASSERT_DELTA(  pw->getPeak(2).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_EQUALS( pw->getPeak(3).getDetectorID(), 2);
    TS_ASSERT_DELTA(  pw->getPeak(3).getWavelength(), 3.0, 1e-5);

    // Sort by wavelength ascending then detID
    criteria.clear();
    criteria.push_back( std::pair<std::string, bool>("wavelength", true) );
    criteria.push_back( std::pair<std::string, bool>("detid", true) );
    pw->sort(criteria);
    TS_ASSERT_EQUALS( pw->getPeak(0).getDetectorID(), 1);
    TS_ASSERT_DELTA(  pw->getPeak(0).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_EQUALS( pw->getPeak(1).getDetectorID(), 2);
    TS_ASSERT_DELTA(  pw->getPeak(1).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_EQUALS( pw->getPeak(2).getDetectorID(), 3);
    TS_ASSERT_DELTA(  pw->getPeak(2).getWavelength(), 3.0, 1e-5);
    TS_ASSERT_EQUALS( pw->getPeak(3).getDetectorID(), 1);
    TS_ASSERT_DELTA(  pw->getPeak(3).getWavelength(), 4.0, 1e-5);
    TS_ASSERT_EQUALS( pw->getPeak(4).getDetectorID(), 1);
    TS_ASSERT_DELTA(  pw->getPeak(4).getWavelength(), 5.0, 1e-5);

  }


};


#endif /* MANTID_DATAOBJECTS_PEAKSWORKSPACETEST_H_ */

