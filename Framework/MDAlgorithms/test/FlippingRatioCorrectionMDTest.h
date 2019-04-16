// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_FLIPPINGRATIOCORRECTIONMDTEST_H_
#define MANTID_MDALGORITHMS_FLIPPINGRATIOCORRECTIONMDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/FlippingRatioCorrectionMD.h"
#include "MantidMDAlgorithms/CreateMDWorkspace.h"
#include "MantidMDAlgorithms/MergeMD.h"
#include "MantidMDAlgorithms/FakeMDEventData.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"

using Mantid::MDAlgorithms::FlippingRatioCorrectionMD;

class FlippingRatioCorrectionMDTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FlippingRatioCorrectionMDTest *createSuite() { return new FlippingRatioCorrectionMDTest(); }
  static void destroySuite( FlippingRatioCorrectionMDTest *suite ) { delete suite; }


  void test_Init()
  {
    FlippingRatioCorrectionMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_exec()
  {
    createMergedMDWorkspace();

/*
    FlippingRatioCorrectionMD alg;
    // Don't put output in ADS by default
    alg.setChild(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", inputWS) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", "_unused_for_child") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from the algorithm. The type here will probably need to change. It should
    // be the type using in declareProperty for the "OutputWorkspace" type.
    // We can't use auto as it's an implicit conversion.
    Workspace_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    TS_FAIL("TODO: Check the results and remove this line");
    */
  }
  

private:
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
        algMerge.setPropertyValue("InputWorkspaces",md1Name+","+md2Name));
    TS_ASSERT_THROWS_NOTHING(algMerge.setPropertyValue("OutputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(algMerge.execute(););
    TS_ASSERT(algMerge.isExecuted());
  }

  void createMDWorkspace(const std::string &wsName, const std::string &peakParams, const double sampleLog) {
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
        Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::IMDEventWorkspace>(wsName);
    Mantid::API::ExperimentInfo_sptr ei(new Mantid::API::ExperimentInfo);
    out->addExperimentInfo(ei);
    out->getExperimentInfo(0)->mutableRun().addProperty("prop", sampleLog);
    // Add peak data
    Mantid::MDAlgorithms::FakeMDEventData algFake;
    TS_ASSERT_THROWS_NOTHING(algFake.initialize())
    TS_ASSERT(algFake.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algFake.setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(algFake.setPropertyValue("PeakParams", peakParams));
    algFake.execute();
  }

};


#endif /* MANTID_MDALGORITHMS_FLIPPINGRATIOCORRECTIONMDTEST_H_ */
