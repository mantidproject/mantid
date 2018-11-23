// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CALCULATEDYNAMICRANGETEST_H_
#define MANTID_ALGORITHMS_CALCULATEDYNAMICRANGETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAlgorithms/CalculateDynamicRange.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidDataHandling/MaskDetectorsInShape.h"
#include "MantidDataHandling/MoveInstrumentComponent.h"

#include <boost/cast.hpp>

using Mantid::API::FrameworkManager;
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::Workspace;
using Mantid::API::Workspace_sptr;
using Mantid::Algorithms::CalculateDynamicRange;
using Mantid::Algorithms::CreateSampleWorkspace;
using Mantid::DataHandling::MaskDetectorsInShape;
using Mantid::DataHandling::MoveInstrumentComponent;

class CalculateDynamicRangeTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateDynamicRangeTest *createSuite() {
    return new CalculateDynamicRangeTest();
  }
  static void destroySuite(CalculateDynamicRangeTest *suite) { delete suite; }

  CalculateDynamicRangeTest() { FrameworkManager::Instance(); }

  void test_init() {
    CalculateDynamicRange alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    CalculateDynamicRange alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    MatrixWorkspace_sptr ws = create_workspace();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    ws = alg.getProperty("Workspace");
    TS_ASSERT(ws);
    const auto &run = ws->run();
    TS_ASSERT(run.hasProperty("qmin"))
    TS_ASSERT(run.hasProperty("qmax"))
    const double qmin = run.getPropertyAsSingleValue("qmin");
    const double qmax = run.getPropertyAsSingleValue("qmax");
    TS_ASSERT_DELTA(qmin, 0.03701, 1E-5)
    TS_ASSERT_DELTA(qmax, 0.73499, 1E-5)
  }

  void test_components() {
    CalculateDynamicRange alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setChild(true);
    MatrixWorkspace_sptr ws = create_workspace(2);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace", ws))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "ComponentNames", std::vector<std::string>{"bank1", "bank2"}))
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    ws = alg.getProperty("Workspace");
    TS_ASSERT(ws);
    const auto &run = ws->run();
    TS_ASSERT(run.hasProperty("qmin"))
    TS_ASSERT(run.hasProperty("qmax"))
    double qmin = run.getPropertyAsSingleValue("qmin");
    double qmax = run.getPropertyAsSingleValue("qmax");
    TS_ASSERT_DELTA(qmin, 0.01851, 1E-5)
    TS_ASSERT_DELTA(qmax, 0.73499, 1E-5)
    TS_ASSERT(run.hasProperty("qmin_bank1"))
    TS_ASSERT(run.hasProperty("qmax_bank1"))
    qmin = run.getPropertyAsSingleValue("qmin_bank1");
    qmax = run.getPropertyAsSingleValue("qmax_bank1");
    TS_ASSERT_DELTA(qmin, 0.03701, 1E-5)
    TS_ASSERT_DELTA(qmax, 0.73499, 1E-5)
    TS_ASSERT(run.hasProperty("qmin_bank2"))
    TS_ASSERT(run.hasProperty("qmax_bank2"))
    qmin = run.getPropertyAsSingleValue("qmin_bank2");
    qmax = run.getPropertyAsSingleValue("qmax_bank2");
    TS_ASSERT_DELTA(qmin, 0.01851, 1E-5)
    TS_ASSERT_DELTA(qmax, 0.66242, 1E-5)
  }

private:
  MatrixWorkspace_sptr create_workspace(const int numBanks = 1) {
    CreateSampleWorkspace creator;
    creator.initialize();
    creator.setChild(true);
    creator.setPropertyValue("OutputWorkspace", "__unused");
    creator.setProperty("XUnit", "Wavelength");
    creator.setProperty("NumBanks", numBanks);
    creator.setProperty("PixelSpacing", 0.1);
    creator.setProperty("XMin", 1.);
    creator.setProperty("XMax", 5.);
    creator.setProperty("BinWidth", 0.4);
    creator.execute();
    MatrixWorkspace_sptr sampleWS = creator.getProperty("OutputWorkspace");
    MoveInstrumentComponent mover;
    mover.initialize();
    mover.setChild(true);
    mover.setProperty("Workspace", sampleWS);
    mover.setProperty("ComponentName", "bank1");
    mover.setProperty("RelativePosition", true);
    mover.setProperty("Y", -0.5);
    mover.setProperty("X", -0.5);
    mover.execute();
    Workspace_sptr movedWS = mover.getProperty("Workspace");
    const std::string shapeXML = "<infinite-cylinder id ='A'>"
                                 "<centre x ='0' y ='0' z ='0'/>"
                                 "<axis x = '0' y = '0' z = '1'/>"
                                 "<radius val = '0.1'/>"
                                 "</infinite-cylinder>";
    MaskDetectorsInShape masker;
    masker.initialize();
    masker.setChild(true);
    masker.setProperty("Workspace", movedWS);
    masker.setPropertyValue("ShapeXML", shapeXML);
    masker.execute();
    MatrixWorkspace_sptr maskedWS = masker.getProperty("Workspace");
    return maskedWS;
  }
};

#endif /* MANTID_ALGORITHMS_CALCULATEDYNAMICRANGETEST_H_ */
