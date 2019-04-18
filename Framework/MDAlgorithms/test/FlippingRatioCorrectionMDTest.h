// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_FLIPPINGRATIOCORRECTIONMDTEST_H_
#define MANTID_MDALGORITHMS_FLIPPINGRATIOCORRECTIONMDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidMDAlgorithms/BinMD.h"
#include "MantidMDAlgorithms/CreateMDWorkspace.h"
#include "MantidMDAlgorithms/FakeMDEventData.h"
#include "MantidMDAlgorithms/FlippingRatioCorrectionMD.h"
#include "MantidMDAlgorithms/MergeMD.h"

using Mantid::MDAlgorithms::FlippingRatioCorrectionMD;

class FlippingRatioCorrectionMDTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FlippingRatioCorrectionMDTest *createSuite() {
    return new FlippingRatioCorrectionMDTest();
  }
  static void destroySuite(FlippingRatioCorrectionMDTest *suite) {
    delete suite;
  }

  void test_Init() {
    FlippingRatioCorrectionMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec_constFR() {
    createMergedMDWorkspace();

    FlippingRatioCorrectionMD alg;
    std::string outWS1Name("Output1"), outWS2Name("Output2");
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InputWorkspace", "MergedWSForFR"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FlippingRatio", "10."));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace1", outWS1Name));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace2", outWS2Name));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // first correction is F/(F-1)
    checkFRCorrection(outWS1Name, 10. / 9., 10. / 9.);
    // second correction is 1/(F-1)
    checkFRCorrection(outWS2Name, 1. / 9., 1. / 9.);
  }

  void test_exec_variableFR() {
    createMergedMDWorkspace();

    FlippingRatioCorrectionMD alg;
    std::string outWS1Name("Output1"), outWS2Name("Output2");
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InputWorkspace", "MergedWSForFR"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FlippingRatio", "prop*pi"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SampleLogs", "prop"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace1", outWS1Name));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace2", outWS2Name));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // first correction is F/(F-1)
    double F1 = 10. * M_PI, F2 = 20. * M_PI;
    checkFRCorrection(outWS1Name, F1 / (F1 - 1.), F2 / (F2 - 1.));
    // second correction is 1/(F-1)
    checkFRCorrection(outWS2Name, 1. / (F1 - 1.), 1. / (F2 - 1.));
  }

private:
  void checkFRCorrection(std::string wsName, double expectedValuePeak1,
                         double expectedValuePeak2) {
    Mantid::MDAlgorithms::BinMD algBin;
    TS_ASSERT_THROWS_NOTHING(algBin.initialize())
    TS_ASSERT(algBin.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algBin.setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(algBin.setProperty("AxisAligned", true));
    TS_ASSERT_THROWS_NOTHING(algBin.setProperty("AlignedDim0", "A,-5,5,2"));
    TS_ASSERT_THROWS_NOTHING(algBin.setProperty("AlignedDim1", "B,-5,5,2"));
    TS_ASSERT_THROWS_NOTHING(
        algBin.setPropertyValue("OutputWorkspace", wsName + "bin"));
    algBin.execute();
    TS_ASSERT(algBin.isExecuted());
    Mantid::DataObjects::MDHistoWorkspace_sptr out;
    TS_ASSERT_THROWS_NOTHING(
        out =
            boost::dynamic_pointer_cast<Mantid::DataObjects::MDHistoWorkspace>(
                Mantid::API::AnalysisDataService::Instance().retrieve(wsName +
                                                                      "bin"));)
    TS_ASSERT(out);
    auto sig = out->getSignalDataVector();
    TS_ASSERT_DELTA(out->getSignalAt(0, 0), 100. * expectedValuePeak1, 1e-5);
    TS_ASSERT_DELTA(out->getSignalAt(0, 1), 0., 1e-5);
    TS_ASSERT_DELTA(out->getSignalAt(1, 0), 0., 1e-5);
    TS_ASSERT_DELTA(out->getSignalAt(1, 1), 100. * expectedValuePeak2, 1e-5);
  }

  void createMergedMDWorkspace() {
    std::string md1Name = "__temp_InputMDWorkspaceName1";
    std::string md2Name = "__temp_InputMDWorkspaceName2";
    std::string wsName = "MergedWSForFR";
    std::string peak1 = "100, -2,-2,0.5";
    std::string peak2 = "100, 2,2,0.5";
    double param1 = 10., param2 = 20.;

    createMDWorkspace(md1Name, peak1, param1);
    createMDWorkspace(md2Name, peak2, param2);
    Mantid::MDAlgorithms::MergeMD algMerge;
    TS_ASSERT_THROWS_NOTHING(algMerge.initialize())
    TS_ASSERT(algMerge.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        algMerge.setPropertyValue("InputWorkspaces", md1Name + "," + md2Name));
    TS_ASSERT_THROWS_NOTHING(
        algMerge.setPropertyValue("OutputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(algMerge.execute(););
    TS_ASSERT(algMerge.isExecuted());
  }

  void createMDWorkspace(const std::string &wsName,
                         const std::string &peakParams,
                         const double sampleLog) {
    // Create Workspace
    const int ndims = 2;
    std::string extents = "-5,5,-5,5";
    std::vector<std::string> names(ndims);
    names[0] = "A";
    names[1] = "B";
    std::vector<std::string> units(ndims);
    units[0] = "a";
    units[1] = "a";
    Mantid::MDAlgorithms::CreateMDWorkspace alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Dimensions", 2));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("EventType", "MDEvent"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Extents", extents));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Names", names));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Units", units));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", wsName));
    alg.execute();
    // Add log
    Mantid::API::IMDEventWorkspace_sptr out =
        boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(wsName));
    Mantid::API::ExperimentInfo_sptr ei(new Mantid::API::ExperimentInfo);
    out->addExperimentInfo(ei);
    out->getExperimentInfo(0)->mutableRun().addProperty("prop", sampleLog);
    // Add peak data
    Mantid::MDAlgorithms::FakeMDEventData algFake;
    TS_ASSERT_THROWS_NOTHING(algFake.initialize())
    TS_ASSERT(algFake.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        algFake.setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(
        algFake.setPropertyValue("PeakParams", peakParams));
    algFake.execute();
  }
};

#endif /* MANTID_MDALGORITHMS_FLIPPINGRATIOCORRECTIONMDTEST_H_ */
