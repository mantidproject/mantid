// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_RECALCULATETRAJECTORIESEXTENTSTEST_H_
#define MANTID_MDALGORITHMS_RECALCULATETRAJECTORIESEXTENTSTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/MDGeometry/QSample.h"
#include "MantidMDAlgorithms/CreateMDWorkspace.h"
#include "MantidMDAlgorithms/RecalculateTrajectoriesExtents.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cxxtest/TestSuite.h>

using Mantid::MDAlgorithms::RecalculateTrajectoriesExtents;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using VectorDoubleProperty =
    Mantid::Kernel::PropertyWithValue<std::vector<double>>;

class RecalculateTrajectoriesExtentsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RecalculateTrajectoriesExtentsTest *createSuite() {
    return new RecalculateTrajectoriesExtentsTest();
  }
  static void destroySuite(RecalculateTrajectoriesExtentsTest *suite) {
    delete suite;
  }

  IMDEventWorkspace_sptr create_workspace(std::vector<double> extents,
                                          std::string name) {
    // ---- empty MDEW ----
    TS_ASSERT_EQUALS(extents.size(), 6)
    CreateMDWorkspace algC;
    algC.initialize();
    algC.setProperty("Dimensions", "3");
    algC.setProperty("Extents", extents);
    std::string frames = Mantid::Geometry::QSample::QSampleName + "," +
                         Mantid::Geometry::QSample::QSampleName + "," +
                         Mantid::Geometry::QSample::QSampleName;
    algC.setProperty("Frames", frames);
    algC.setPropertyValue("Names", "x,y,z");
    algC.setPropertyValue("Units", "m,mm,um");
    algC.setPropertyValue("OutputWorkspace", name);
    algC.execute();
    IMDEventWorkspace_sptr out = boost::dynamic_pointer_cast<IMDEventWorkspace>(
        AnalysisDataService::Instance().retrieve(name));
    TS_ASSERT(out);

    std::vector<double> L2{1, 1, 1}, pol{0.1, 0.2, 0.3}, azi{0, 1, 2};
    Instrument_sptr inst =
        ComponentCreationHelper::createCylInstrumentWithDetInGivenPositions(
            L2, pol, azi);
    inst->setName("Test");

    ExperimentInfo_sptr ei =
        ExperimentInfo_sptr(new Mantid::API::ExperimentInfo());
    ei->setInstrument(inst);
    std::vector<double> high(3, 3), low(3, 1);
    ei->mutableRun().addProperty("MDNorm_high", high);
    ei->mutableRun().addProperty("MDNorm_low", low);
    out->addExperimentInfo(ei);
    TS_ASSERT_EQUALS(out->getNumExperimentInfo(), 1)
    return out;
  }
  void test_Init() {
    RecalculateTrajectoriesExtents alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void do_test(std::string name, std::vector<double> extents) {
    IMDEventWorkspace_sptr inputWS = create_workspace(extents, name);

    RecalculateTrajectoriesExtents alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", name));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from the algorithm. The type here will probably
    // need to change. It should be the type using in declareProperty for the
    // "OutputWorkspace" type. We can't use auto as it's an implicit conversion.
    IMDEventWorkspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    auto ei = outputWS->getExperimentInfo(0);
    TS_ASSERT(ei);
    auto *lowValuesLog =
        dynamic_cast<VectorDoubleProperty *>(ei->getLog("MDNorm_low"));
    auto *highValuesLog =
        dynamic_cast<VectorDoubleProperty *>(ei->getLog("MDNorm_high"));
    const auto &spectrumInfo = ei->spectrumInfo();
    for (size_t i = 0; i < 3; i++) {
      const auto &detector = spectrumInfo.detector(i);
      double theta = detector.getTwoTheta(Mantid::Kernel::V3D(0, 0, 0),
                                          Mantid::Kernel::V3D(0, 0, 1));
      double phi = detector.getPhi();
      double Qx = -sin(theta) * cos(phi);
      double lamMin = (*lowValuesLog)()[i];
      double lamMax = ((*highValuesLog)()[i]);
      // correct answer if the trajectory is all outside the box
      // otherwise both ends must be in or on the box
      if (lamMin != lamMax) {
        // float has 7 digits of precision
        TS_ASSERT_LESS_THAN_EQUALS((Qx * lamMin - extents[0]) *
                                       (Qx * lamMin - extents[1]),
                                   1e-7 * (extents[1] - extents[0]))
        TS_ASSERT_LESS_THAN_EQUALS((Qx * lamMax - extents[0]) *
                                       (Qx * lamMax - extents[1]),
                                   1e-7 * (extents[1] - extents[0]))
      }
    }
    AnalysisDataService::Instance().remove(name);
  }

  void test_exec_no_cut() {
    std::vector<double> extents{-10, 10, -10, 10, -10, 10};
    std::string name("RecalculateTrajectoriesExtents_no_cut_test");
    do_test(name, extents);
  }

  void test_exec_cut() {
    std::vector<double> extents{-0.2, 10, -10, 10, -10, 10};
    std::string name("RecalculateTrajectoriesExtents_cut_test");
    do_test(name, extents);
  }
};

#endif /* MANTID_MDALGORITHMS_RECALCULATETRAJECTORIESEXTENTSTEST_H_ */
