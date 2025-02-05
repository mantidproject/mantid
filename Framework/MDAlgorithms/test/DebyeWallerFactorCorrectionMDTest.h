// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/MDEventInserter.h"
#include "MantidMDAlgorithms/DebyeWallerFactorCorrectionMD.h"
#include "QTransformTest.h"

using Mantid::MDAlgorithms::DebyeWallerFactorCorrectionMD;

class DebyeWallerFactorCorrectionMDTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DebyeWallerFactorCorrectionMDTest *createSuite() { return new DebyeWallerFactorCorrectionMDTest(); }
  static void destroySuite(DebyeWallerFactorCorrectionMDTest *suite) { delete suite; }

  void test_exec_1D() {
    // create a 1D MD workspace
    auto create_alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("CreateMDWorkspace");
    create_alg->setChild(true);
    create_alg->initialize();
    create_alg->setProperty("Dimensions", 1);
    create_alg->setProperty("Extents", "1,4");
    create_alg->setProperty("Names", "|Q|");
    create_alg->setProperty("Units", "A");
    create_alg->setPropertyValue("OutputWorkspace", "_unused_for_child");
    create_alg->execute();

    Mantid::API::Workspace_sptr inputWS = create_alg->getProperty("OutputWorkspace");

    // add MD events with Q values 1, 2, 3, 4
    MDEventWorkspace<MDLeanEvent<1>, 1>::sptr mdws_mdevt_1 =
        std::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<1>, 1>>(inputWS);
    MDEventInserter<MDEventWorkspace<MDLeanEvent<1>, 1>::sptr> inserter(mdws_mdevt_1);

    for (Mantid::coord_t q = 1; q < 5; ++q) {
      Mantid::coord_t coord[1] = {q};
      inserter.insertMDEvent(1.0, 1.0, 0, 0, 0, coord);
    }

    DebyeWallerFactorCorrectionMD alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Mean squared displacement", 0.1));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    Mantid::API::IMDEventWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");

    // get the events from the output workspace and check the signal, error, and center values
    auto output_events = QTransformTest::get_events_helper(outputWS);

    TS_ASSERT_EQUALS(output_events.size(), 4);

    for (size_t i = 0; i < output_events.size(); ++i) {
      const double q = static_cast<double>(i + 1);
      const double q2 = q * q;
      TS_ASSERT_DELTA(output_events[i][0], exp(0.1 * q2), 1e-5); // signal
      TS_ASSERT_DELTA(output_events[i][1], exp(0.1 * q2), 1e-5); // error
      TS_ASSERT_EQUALS(output_events[i][2], q);
    }
  }

  void test_exec_3D() {
    // create a 3D MD workspace
    auto create_alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("CreateMDWorkspace");
    create_alg->setChild(true);
    create_alg->initialize();
    create_alg->setProperty("Dimensions", 3);
    create_alg->setProperty("Extents", "0,10,0,10,0,10");
    create_alg->setProperty("Names", "Qx,Qy,Qz");
    create_alg->setProperty("Units", "A,A,A");
    create_alg->setProperty("Frames", "QSample,QSample,QSample");
    create_alg->setPropertyValue("OutputWorkspace", "_unused_for_child");
    create_alg->execute();

    Mantid::API::Workspace_sptr inputWS = create_alg->getProperty("OutputWorkspace");

    // add MD events with Q values (1,1,1), (2,2,2), (3,3,3), (4,4,4)
    MDEventWorkspace<MDLeanEvent<3>, 3>::sptr mdws_mdevt_3 =
        std::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<3>, 3>>(inputWS);
    MDEventInserter<MDEventWorkspace<MDLeanEvent<3>, 3>::sptr> inserter(mdws_mdevt_3);

    for (Mantid::coord_t q = 1; q < 5; ++q) {
      Mantid::coord_t coord[3] = {q, q, q};
      inserter.insertMDEvent(1.0, 1.0, 0, 0, 0, coord);
    }
    DebyeWallerFactorCorrectionMD alg;
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "_unused_for_child"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Mean squared displacement", "0.15"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    Mantid::API::IMDEventWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    auto output_events = QTransformTest::get_events_helper(outputWS);

    TS_ASSERT_EQUALS(output_events.size(), 4);

    for (size_t i = 0; i < output_events.size(); ++i) {
      const double q = static_cast<double>(i + 1);
      const double q2 = 3 * q * q;                                // q2 in single crystal mode is qx^2 + qy^2 +qz^2
      TS_ASSERT_DELTA(output_events[i][0], exp(0.15 * q2), 1e-3); // signal
      TS_ASSERT_DELTA(output_events[i][1], exp(0.15 * q2), 1e-3); // error
      TS_ASSERT_EQUALS(output_events[i][2], q);                   // center x
      TS_ASSERT_EQUALS(output_events[i][3], q);                   // center y
      TS_ASSERT_EQUALS(output_events[i][4], q);                   // center z
    }
  }
};
