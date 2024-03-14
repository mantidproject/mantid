// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidDataHandling/LoadErrorEventsNexus.h"
#include "MantidDataObjects/EventWorkspace.h"

using Mantid::DataHandling::LoadErrorEventsNexus;

class LoadErrorEventsNexusTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadErrorEventsNexusTest *createSuite() { return new LoadErrorEventsNexusTest(); }
  static void destroySuite(LoadErrorEventsNexusTest *suite) { delete suite; }

  void test_REF_L() {
    LoadErrorEventsNexus alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "unused"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "REF_L_183110.nxs.h5"))
    TS_ASSERT(alg.execute());

    Mantid::DataObjects::EventWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);

    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1)
    TS_ASSERT_EQUALS(outputWS->getNumberEvents(), 82980)
    TS_ASSERT_DELTA(outputWS->readX(0)[0], 8494.900394, 1e-5)
    TS_ASSERT_DELTA(outputWS->readX(0)[1], 24927.699219, 1e-5)
  }

  void test_CG3() {
    LoadErrorEventsNexus alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "unused"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "CG3_13118.nxs.h5"))
    TS_ASSERT(alg.execute());

    Mantid::DataObjects::EventWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);

    TS_ASSERT_EQUALS(outputWS->blocksize(), 1)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 1)
    TS_ASSERT_EQUALS(outputWS->getNumberEvents(), 6052)
    TS_ASSERT_DELTA(outputWS->readX(0)[0], 0.5, 1e-5)
    TS_ASSERT_DELTA(outputWS->readX(0)[1], 16663.0996, 1e-5)
  }

  void test_HYSA() {
    // this should fail to load as bank_error_events does not exist in this file

    LoadErrorEventsNexus alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "unused"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", "HYSA_12509.nxs.h5"))
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &)
    TS_ASSERT(!alg.isExecuted())
  }
};
