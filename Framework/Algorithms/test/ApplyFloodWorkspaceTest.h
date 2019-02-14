// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_APPLYFLOODWORKSPACETEST_H_
#define MANTID_ALGORITHMS_APPLYFLOODWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/ApplyFloodWorkspace.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidKernel/Unit.h"
#include "MantidTestHelpers/ReflectometryHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace WorkspaceCreationHelper;

class ApplyFloodWorkspaceTest : public CxxTest::TestSuite {
public:
  void test_flood_same_x_units() {
    auto inputWS =
        create2DWorkspaceWithReflectometryInstrumentMultiDetector(0, 0.1);
    auto flood = createFloodWorkspace(inputWS->getInstrument());

    ApplyFloodWorkspace alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("FloodWorkspace", flood);
    alg.setProperty("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr out = alg.getProperty("OutputWorkspace");
    TS_ASSERT_DELTA(out->y(0)[0], 2.8571428575, 1e-9);
    TS_ASSERT_DELTA(out->y(1)[0], 2.0, 1e-9);
    TS_ASSERT_DELTA(out->y(2)[0], 2.5, 1e-9);
    TS_ASSERT_DELTA(out->y(3)[0], 2.2222222222, 1e-9);
    AnalysisDataService::Instance().clear();
  }

  void test_flood_different_x_units() {
    auto inputWS =
        create2DWorkspaceWithReflectometryInstrumentMultiDetector(0, 0.1);
    auto flood = createFloodWorkspace(inputWS->getInstrument(), "Wavelength");

    ApplyFloodWorkspace alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inputWS);
    alg.setProperty("FloodWorkspace", flood);
    alg.setProperty("OutputWorkspace", "dummy");
    alg.execute();
    MatrixWorkspace_sptr out = alg.getProperty("OutputWorkspace");
    TS_ASSERT_DELTA(out->y(0)[0], 2.8571428575, 1e-9);
    TS_ASSERT_DELTA(out->y(1)[0], 2.0, 1e-9);
    TS_ASSERT_DELTA(out->y(2)[0], 2.5, 1e-9);
    TS_ASSERT_DELTA(out->y(3)[0], 2.2222222222, 1e-9);
    AnalysisDataService::Instance().clear();
  }

private:
  MatrixWorkspace_sptr
  createFloodWorkspace(Mantid::Geometry::Instrument_const_sptr instrument,
                       std::string const &xUnit = "TOF") {
    MatrixWorkspace_sptr flood = create2DWorkspace(4, 1);
    flood->mutableY(0)[0] = 0.7;
    flood->mutableY(1)[0] = 1.0;
    flood->mutableY(2)[0] = 0.8;
    flood->mutableY(3)[0] = 0.9;
    flood->setInstrument(instrument);
    for (size_t i = 0; i < flood->getNumberHistograms(); ++i) {
      flood->getSpectrum(i).setDetectorID(Mantid::detid_t(i + 1));
    }
    flood->getAxis(0)->setUnit("TOF");
    if (xUnit != "TOF") {
      ConvertUnits convert;
      convert.initialize();
      convert.setChild(true);
      convert.setProperty("InputWorkspace", flood);
      convert.setProperty("Target", xUnit);
      convert.setProperty("OutputWorkspace", "dummy");
      convert.execute();
      flood = convert.getProperty("OutputWorkspace");
    }
    return flood;
  }
};

#endif /* MANTID_ALGORITHMS_APPLYFLOODWORKSPACETEST_H_ */
