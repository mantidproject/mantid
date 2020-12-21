// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <stdexcept>

#include "MantidCrystal/SCDCalibratePanels2.h"


using Mantid::Crystal::SCDCalibratePanels2;

class SCDCalibratePanels2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SCDCalibratePanels2Test *createSuite() { return new SCDCalibratePanels2Test(); }
  static void destroySuite( SCDCalibratePanels2Test *suite ) { delete suite; }

  void testName() {
    SCDCalibratePanels2 alg;
    TS_ASSERT_EQUALS(alg.name(), "SCDCalibratePanels2");
  }

  void testInit(){
    SCDCalibratePanels2 alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
  }

  //TODO: the remaining test needs to be implemented
  // ///TODO: test validators

  ///NULL case
  void testNullCase(){
    SCDCalibratePanels2 alg;
  }

  ///Adjust T0 and L1
  void testGlobalShiftOnly(){
    SCDCalibratePanels2 alg;
  }

  ///Ideal global with one panels moved
  void testSinglePanelMoved(){
    SCDCalibratePanels2 alg;
  }

  ///Ideal global with two panels moved
  void testDualPanelMoved(){
    SCDCalibratePanels2 alg;
  }

  // Test with mocked CORELLI instrument
  // T0, L1 adjusted
  // Two panels moved
  void testExec(){
    SCDCalibratePanels2 alg;
  }

  /// Helper functions for unittest

  // generate a peaktable with given movement void

};
