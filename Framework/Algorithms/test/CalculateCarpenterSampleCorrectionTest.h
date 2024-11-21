// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <vector>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/CalculateCarpenterSampleCorrection.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/WarningSuppressions.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

class CalculateCarpenterSampleCorrectionTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(algorithm.name(), "CalculateCarpenterSampleCorrection"); }

  void testVersion() { TS_ASSERT_EQUALS(algorithm.version(), 1); }

  void testInit() {
    Mantid::Algorithms::CalculateCarpenterSampleCorrection algorithm_b;
    TS_ASSERT_THROWS_NOTHING(algorithm_b.initialize());
    TS_ASSERT(algorithm_b.isInitialized());

    const std::vector<Property *> props = algorithm_b.getProperties();
    TS_ASSERT_EQUALS(props.size(), 8);

    TS_ASSERT_EQUALS(props[0]->name(), "InputWorkspace");
    TS_ASSERT(props[0]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<MatrixWorkspace> *>(props[0]));

    TS_ASSERT_EQUALS(props[1]->name(), "OutputWorkspaceBaseName");
    TS_ASSERT(props[1]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<API::WorkspaceGroup> *>(props[1]));

    TS_ASSERT_EQUALS(props[2]->name(), "AttenuationXSection");
    TS_ASSERT(props[2]->isDefault());
    TS_ASSERT(dynamic_cast<PropertyWithValue<double> *>(props[2]));

    TS_ASSERT_EQUALS(props[3]->name(), "ScatteringXSection");
    TS_ASSERT(props[3]->isDefault());
    TS_ASSERT(dynamic_cast<PropertyWithValue<double> *>(props[3]));

    TS_ASSERT_EQUALS(props[4]->name(), "SampleNumberDensity");
    TS_ASSERT(props[4]->isDefault());
    TS_ASSERT(dynamic_cast<PropertyWithValue<double> *>(props[4]));

    TS_ASSERT_EQUALS(props[5]->name(), "CylinderSampleRadius");
    TS_ASSERT(props[5]->isDefault());
    TS_ASSERT(dynamic_cast<PropertyWithValue<double> *>(props[5]));

    TS_ASSERT_EQUALS(props[6]->name(), "Absorption");
    TS_ASSERT(props[6]->isDefault());
    TS_ASSERT(dynamic_cast<PropertyWithValue<bool> *>(props[6]));

    TS_ASSERT_EQUALS(props[7]->name(), "MultipleScattering");
    TS_ASSERT(props[7]->isDefault());
    TS_ASSERT(dynamic_cast<PropertyWithValue<bool> *>(props[7]));
  }

  void testCalculationHist() {
    using namespace Mantid::HistogramData;
    auto wksp = DataObjects::create<DataObjects::Workspace2D>(
        ComponentCreationHelper::createTestInstrumentCylindrical(1), Indexing::IndexInfo(9),
        Histogram(BinEdges(17, LinearGenerator(1000.0, 1000.0)), Counts(16, 2.0)));
    wksp->getAxis(0)->setUnit("TOF");
    AnalysisDataService::Instance().add("TestInputWS", std::move(wksp));

    // convert to wavelength
    auto convertUnitsAlg = Mantid::API::AlgorithmManager::Instance().create("ConvertUnits");
    convertUnitsAlg->setPropertyValue("InputWorkspace", "TestInputWS");
    convertUnitsAlg->setPropertyValue("OutputWorkspace", "TestInputWS");
    convertUnitsAlg->setProperty("Target", "Wavelength");
    convertUnitsAlg->execute();

    // create and execute the algorithm
    Mantid::Algorithms::CalculateCarpenterSampleCorrection algorithm_c;
    TS_ASSERT_THROWS_NOTHING(algorithm_c.initialize());
    TS_ASSERT(algorithm_c.isInitialized());

    TS_ASSERT_THROWS_NOTHING(algorithm_c.setPropertyValue("InputWorkspace", "TestInputWS"));
    TS_ASSERT_THROWS_NOTHING(algorithm_c.setPropertyValue("OutputWorkspaceBaseName", "TestOutputWS"));
    TS_ASSERT_THROWS_NOTHING(algorithm_c.setPropertyValue("CylinderSampleRadius", "0.3175"));
    TS_ASSERT_THROWS_NOTHING(algorithm_c.setPropertyValue("AttenuationXSection", "2.8"));
    TS_ASSERT_THROWS_NOTHING(algorithm_c.setPropertyValue("SampleNumberDensity", "0.0721"));
    TS_ASSERT_THROWS_NOTHING(algorithm_c.setPropertyValue("ScatteringXSection", "5.1"));

    TS_ASSERT_THROWS_NOTHING(algorithm_c.execute());
    TS_ASSERT(algorithm_c.isExecuted());

    WorkspaceGroup_sptr test_output_WS;

    TS_ASSERT_THROWS_NOTHING(test_output_WS =
                                 AnalysisDataService::Instance().retrieveWS<API::WorkspaceGroup>("TestOutputWS"));
    TS_ASSERT(test_output_WS);

    // Check the correction workspaces in the group
    Workspace_sptr absPtr = test_output_WS->getItem(0);
    Workspace_sptr msPtr = test_output_WS->getItem(1);
    auto absWksp = std::dynamic_pointer_cast<MatrixWorkspace>(absPtr);
    auto msWksp = std::dynamic_pointer_cast<MatrixWorkspace>(msPtr);
    TS_ASSERT(absWksp);
    TS_ASSERT(msWksp);

    // Check absorption correction
    const size_t size = 16;
    std::array<double, size> abs_corr_expected = {{0.786608, 0.764593, 0.743221, 0.722473, 0.702329, 0.682772, 0.663783,
                                                   0.645345, 0.627442, 0.610057, 0.593173, 0.576775, 0.560848, 0.545376,
                                                   0.530345, 0.515739}};

    auto &abs_corr_actual = absWksp->y(0);
    for (size_t i = 0; i < size; i++)
      TS_ASSERT_DELTA(abs_corr_actual[i], abs_corr_expected[i], 0.00001);

    // Check applying absorption correction
    auto divide = Mantid::API::AlgorithmManager::Instance().create("Divide");
    divide->initialize();
    divide->setPropertyValue("LHSWorkspace", "TestInputWS");
    divide->setPropertyValue("RHSWorkspace", absWksp->getName());
    divide->setPropertyValue("OutputWorkspace", "TestAbsWS");
    divide->execute();
    MatrixWorkspace_sptr absCorrectedWksp =
        AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>("TestAbsWS");
    std::array<double, size> abs_ws_expected = {{2.54256, 2.61577, 2.69099, 2.76827, 2.84767, 2.92924, 3.01303, 3.09912,
                                                 3.18754, 3.27838, 3.37170, 3.46756, 3.56603, 3.66720, 3.77113,
                                                 3.87793}};
    auto &abs_ws_actual = absCorrectedWksp->y(0);
    for (size_t i = 0; i < size; i++)
      TS_ASSERT_DELTA(abs_ws_actual[i], abs_ws_expected[i], 0.00001);

    // Check multiple scattering correction
    std::array<double, size> ms_corr_expected = {{0.159334, 0.161684, 0.164032, 0.166376, 0.168712, 0.171039, 0.173355,
                                                  0.175658, 0.177944, 0.180211, 0.182457, 0.184678, 0.186873, 0.189038,
                                                  0.191171, 0.193268}};
    auto &ms_corr_actual = msWksp->y(0);
    for (size_t i = 0; i < size; i++)
      TS_ASSERT_DELTA(ms_corr_actual[i], ms_corr_expected[i], 0.00001);

    // Check applying multiple scattering correction
    auto multiply = Mantid::API::AlgorithmManager::Instance().create("Multiply");
    multiply->initialize();
    multiply->setPropertyValue("LHSWorkspace", "TestInputWS");
    multiply->setPropertyValue("RHSWorkspace", msWksp->getName());
    multiply->setPropertyValue("OutputWorkspace", "TestMultScatWS");
    multiply->execute();
    MatrixWorkspace_sptr msCorrectedWksp =
        AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>("TestMultScatWS");
    std::array<double, size> ms_ws_expected = {{0.318668, 0.323369, 0.328065, 0.332751, 0.337424, 0.342079, 0.346711,
                                                0.351315, 0.355887, 0.360422, 0.364913, 0.369356, 0.373746, 0.378076,
                                                0.382341, 0.386535}};

    auto &ms_ws_actual = msCorrectedWksp->y(0);
    for (size_t i = 0; i < size; i++)
      TS_ASSERT_DELTA(ms_ws_actual[i], ms_ws_expected[i], 0.00001);

    // Check full correction comparison
    auto minus = Mantid::API::AlgorithmManager::Instance().create("Minus");
    minus->initialize();
    minus->setPropertyValue("LHSWorkspace", "TestAbsWS");
    minus->setPropertyValue("RHSWorkspace", "TestMultScatWS");
    minus->setPropertyValue("OutputWorkspace", "TestOutputWS");
    minus->execute();
    MatrixWorkspace_sptr outputWksp = AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>("TestOutputWS");
    std::array<double, size> test_ws_expected = {{2.22389, 2.2924, 2.36292, 2.43552, 2.51024, 2.58716, 2.66632, 2.7478,
                                                  2.83166, 2.91796, 3.00678, 3.09820, 3.19228, 3.28912, 3.38879,
                                                  3.49139}};

    auto &test_ws_actual = outputWksp->y(0);
    for (size_t i = 0; i < size; i++)
      TS_ASSERT_DELTA(test_ws_actual[i], test_ws_expected[i], 0.00001);

    // cleanup
    AnalysisDataService::Instance().remove("TestInputWS");
    AnalysisDataService::Instance().remove("TestAbsWS");
    AnalysisDataService::Instance().remove("TestMultScatWS");
    AnalysisDataService::Instance().remove("TestOutputWS");
  }

  GNU_DIAG_OFF("dangling-reference")

  void testCalculationEvent() {
    const std::string outName("CalculateCarpenterSampleCorrectionEventOutput");

    // setup the test workspace
    auto wksp = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(1, 1, false);
    wksp->getAxis(0)->setUnit("Wavelength"); // cheat and set the units to Wavelength
    wksp->getSpectrum(0).convertTof(.09,
                                    1.); // convert to be from 1->10 (about)

    AnalysisDataService::Instance().add(outName, wksp);

    // create the algorithm
    Mantid::Algorithms::CalculateCarpenterSampleCorrection algorithm;
    TS_ASSERT_THROWS_NOTHING(algorithm.initialize());
    TS_ASSERT(algorithm.isInitialized());

    // execute the algorithm
    TS_ASSERT_THROWS_NOTHING(algorithm.setProperty("InputWorkspace", wksp));
    TS_ASSERT_THROWS_NOTHING(algorithm.setPropertyValue("OutputWorkspaceBaseName", outName));
    TS_ASSERT_THROWS_NOTHING(algorithm.execute());
    TS_ASSERT(algorithm.isExecuted());

    // quick checks on the output workspace
    WorkspaceGroup_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(outputWS = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outName));
    // Check the correction workspaces in the group
    auto absWksp = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(0));
    auto msWksp = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(1));
    TS_ASSERT(absWksp);
    TS_ASSERT(msWksp);

    auto group = std::dynamic_pointer_cast<WorkspaceGroup>(outputWS);
    TS_ASSERT_EQUALS(group->getNumberOfEntries(), 2);

    // Check absorption correction
    const size_t size = 16;
    std::array<double, size> abs_corr_expected = {{0.733553, 0.726500, 0.719519, 0.712607, 0.705765, 0.698992, 0.692286,
                                                   0.685648, 0.679076, 0.672570, 0.666129, 0.659753, 0.65344, 0.647191,
                                                   0.641004, 0.634878}};
    const auto &abs_corr_actual = absWksp->histogram(0).y();
    for (size_t i = 0; i < size; i++)
      TS_ASSERT_DELTA(abs_corr_actual[i], abs_corr_expected[i], 0.00001);

    const auto &abs_corr_actual_err = absWksp->histogram(0).e();
    for (size_t i = 0; i < size; i++)
      TS_ASSERT_EQUALS(abs_corr_actual_err[i], 0.00);

    // Check multiple scattering correction
    std::array<double, size> ms_corr_expected = {{0.165116, 0.165916, 0.166714, 0.167512, 0.168309, 0.169105, 0.169900,
                                                  0.170693, 0.171486, 0.172277, 0.173066, 0.173854, 0.17464, 0.175425,
                                                  0.176207, 0.176988}};
    auto &ms_corr_actual = msWksp->y(0);
    for (size_t i = 0; i < size; i++)
      TS_ASSERT_DELTA(ms_corr_actual[i], ms_corr_expected[i], 0.00001);

    const auto &ms_corr_actual_err = msWksp->histogram(0).e();
    for (size_t i = 0; i < size; i++)
      TS_ASSERT_EQUALS(ms_corr_actual_err[i], 0.00);

    // cleanup
    AnalysisDataService::Instance().remove(wksp->getName());
    AnalysisDataService::Instance().remove(outName + "_abs");
    AnalysisDataService::Instance().remove(outName + "_ms");
    AnalysisDataService::Instance().remove(outName);
  }

  GNU_DIAG_ON("dangling-reference")

private:
  Mantid::Algorithms::CalculateCarpenterSampleCorrection algorithm;
};
