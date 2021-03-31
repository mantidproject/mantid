// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidSINQ/PoldiUtilities/PoldiDetectorFactory.h"
#include "MantidSINQ/PoldiUtilities/PoldiHeliumDetector.h"

#include "boost/date_time/gregorian/gregorian.hpp"

using namespace Mantid::Poldi;
using namespace boost::gregorian;

class PoldiDetectorFactoryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiDetectorFactoryTest *createSuite() { return new PoldiDetectorFactoryTest(); }
  static void destroySuite(PoldiDetectorFactoryTest *suite) { delete suite; }

  void testDetectorByType() {
    PoldiDetectorFactory detectorFactory;

    PoldiAbstractDetector *detector = detectorFactory.createDetector(std::string("any"));
    TS_ASSERT(detector);

    PoldiHeliumDetector *heliumDetector = dynamic_cast<PoldiHeliumDetector *>(detector);
    TS_ASSERT(heliumDetector);

    delete detector;
  }

  void testDetectorByDate() {
    PoldiDetectorFactory detectorFactory;

    PoldiAbstractDetector *detector = detectorFactory.createDetector(from_string("2014/05/12"));
    TS_ASSERT(detector);
    PoldiHeliumDetector *heliumDetector = dynamic_cast<PoldiHeliumDetector *>(detector);
    TS_ASSERT(heliumDetector);

    delete detector;

    PoldiAbstractDetector *newDetector = detectorFactory.createDetector(from_string("2016/05/12"));
    TS_ASSERT(!newDetector);
  }
};
