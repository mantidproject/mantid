// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ParComponentFactory.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Geometry;

class ParComponentFactoryTest : public CxxTest::TestSuite {
public:
  void test_createDetector() {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    IDetector_const_sptr idet = inst->getDetector(1);
    const Detector *det = std::dynamic_pointer_cast<const Detector>(idet).get();
    ParameterMap *map = new ParameterMap();

    std::shared_ptr<IDetector> pdet;
    TS_ASSERT_THROWS_NOTHING(pdet = ParComponentFactory::createDetector(det, map));
    TS_ASSERT(pdet);
    delete map;
  }

  void test_createInstrument() {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    ParameterMap_sptr map(new ParameterMap());

    Instrument_sptr pinst;
    TS_ASSERT_THROWS_NOTHING(
        pinst = ParComponentFactory::createInstrument(std::dynamic_pointer_cast<Instrument>(inst), map));
    TS_ASSERT(pinst);
  }
};
