// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidCurveFitting/Algorithms/PlotPeakByLogValueHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::CurveFitting::Algorithms;

class PlotPeakByLogValueHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PlotPeakByLogValueHelperTest *createSuite() { return new PlotPeakByLogValueHelperTest(); }
  static void destroySuite(PlotPeakByLogValueHelperTest *suite) { delete suite; }

  PlotPeakByLogValueHelperTest() {}

  void testWorkspaceIndexSpecified() {
    std::string workspaceName = "irs26176_graphite002_red.nxs,i0";

    auto namesList = makeNames(workspaceName, 0, -1);
    auto inputWithWorkspace = namesList[0];

    TS_ASSERT_EQUALS(namesList.size(), 1);
    TS_ASSERT_EQUALS(inputWithWorkspace.wsIdx, 0);
    TS_ASSERT_EQUALS(inputWithWorkspace.spectrumNum, -1);
    TS_ASSERT_EQUALS(inputWithWorkspace.numericValue, -1);
    TS_ASSERT(inputWithWorkspace.ws);
    TS_ASSERT_EQUALS(inputWithWorkspace.name, "irs26176_graphite002_red.nxs");
  }

  void testWorkspaceSpectrumSpecified() {
    std::string workspaceName = "irs26176_graphite002_red.nxs,sp1";

    auto namesList = makeNames(workspaceName, -1, -1);
    auto inputWithWorkspace = namesList[0];

    TS_ASSERT_EQUALS(namesList.size(), 1);
    TS_ASSERT_EQUALS(inputWithWorkspace.wsIdx, 0);
    TS_ASSERT_EQUALS(inputWithWorkspace.spectrumNum, 1);
    TS_ASSERT_EQUALS(inputWithWorkspace.numericValue, -1);
    TS_ASSERT(inputWithWorkspace.ws);
    TS_ASSERT_EQUALS(inputWithWorkspace.name, "irs26176_graphite002_red.nxs");
  }

  void testWorkspaceRangeSpecifiedSpectrumAxis() {
    std::string workspaceName = "irs26176_graphite002_red.nxs,v1.1:3.2";

    auto namesList = makeNames(workspaceName, -1, -1);

    TS_ASSERT_EQUALS(namesList.size(), 2);
    TS_ASSERT_EQUALS(namesList[0].wsIdx, 1);
    TS_ASSERT_EQUALS(namesList[1].wsIdx, 2);
    TS_ASSERT_EQUALS(namesList[0].spectrumNum, 2);
    TS_ASSERT_EQUALS(namesList[1].spectrumNum, 3);
    TS_ASSERT_EQUALS(namesList[0].numericValue, -1);
    TS_ASSERT_EQUALS(namesList[1].numericValue, -1);
    TS_ASSERT(namesList[0].ws);
    TS_ASSERT(namesList[1].ws);
    TS_ASSERT_EQUALS(namesList[0].name, "irs26176_graphite002_red.nxs");
    TS_ASSERT_EQUALS(namesList[1].name, "irs26176_graphite002_red.nxs");
  }

  void testWorkspaceRangeSpecifiedSpectrumAxisOutOfBounds() {
    std::string workspaceName = "irs26176_graphite002_red.nxs,v-1.1:3.2";

    auto namesList = makeNames(workspaceName, -1, -1);
    auto inputWithWorkspace = namesList[0];

    TS_ASSERT_EQUALS(namesList.size(), 1);
    TS_ASSERT_EQUALS(inputWithWorkspace.wsIdx, -1);
    TS_ASSERT_EQUALS(inputWithWorkspace.spectrumNum, -1);
    TS_ASSERT_EQUALS(inputWithWorkspace.numericValue, -1);
    TS_ASSERT(!inputWithWorkspace.ws);
    TS_ASSERT_EQUALS(inputWithWorkspace.name, "");
  }

  void testWorkspaceIndexNumericAxis() {
    std::string workspaceName = "saveNISTDAT_data.nxs,i0";

    auto namesList = makeNames(workspaceName, -1, -1);
    auto inputWithWorkspace = namesList[0];

    TS_ASSERT_EQUALS(namesList.size(), 1);
    TS_ASSERT_EQUALS(inputWithWorkspace.wsIdx, 0);
    TS_ASSERT_EQUALS(inputWithWorkspace.spectrumNum, -1);
    TS_ASSERT_EQUALS(inputWithWorkspace.numericValue, -1);

    TS_ASSERT(inputWithWorkspace.ws);
    TS_ASSERT_EQUALS(inputWithWorkspace.name, "saveNISTDAT_data.nxs");
  }

  void testWorkspaceSpectrumNumericAxis() {
    std::string workspaceName = "saveNISTDAT_data.nxs,sp1";

    auto namesList = makeNames(workspaceName, -1, -1);
    auto inputWithWorkspace = namesList[0];

    TS_ASSERT_EQUALS(namesList.size(), 1);
    TS_ASSERT_EQUALS(inputWithWorkspace.wsIdx, 160);
    TS_ASSERT_EQUALS(inputWithWorkspace.spectrumNum, -1);
    TS_ASSERT_EQUALS(inputWithWorkspace.numericValue, 0);
    TS_ASSERT(inputWithWorkspace.ws);
    TS_ASSERT_EQUALS(inputWithWorkspace.name, "saveNISTDAT_data.nxs");
  }

  void testWorkspaceRangeSpecifiedNumericAxisAll() {
    std::string workspaceName = "saveNISTDAT_data.nxs";

    auto namesList = makeNames(workspaceName, -2, -2);

    TS_ASSERT_EQUALS(namesList.size(), 321);

    TS_ASSERT_EQUALS(namesList[0].wsIdx, 0);
    TS_ASSERT_EQUALS(namesList[0].spectrumNum, -1);
    TS_ASSERT_DELTA(namesList[0].numericValue, -0.16, 1e-3);

    TS_ASSERT_EQUALS(namesList[200].wsIdx, 200);
    TS_ASSERT_DELTA(namesList[200].numericValue, 0.04, 1e-3);
    TS_ASSERT_EQUALS(namesList[200].spectrumNum, -1);

    TS_ASSERT(namesList[0].ws);
    TS_ASSERT_EQUALS(namesList[0].name, "saveNISTDAT_data.nxs");

    TS_ASSERT(namesList[200].ws);
    TS_ASSERT_EQUALS(namesList[200].name, "saveNISTDAT_data.nxs");
  }

  void testWorkspaceRangeSpecifiedNumericAxis() {
    std::string workspaceName = "saveNISTDAT_data.nxs,v-0.01:0.01";

    auto namesList = makeNames(workspaceName, -1, -1);
    auto inputWithWorkspace = namesList[0];

    TS_ASSERT_EQUALS(namesList.size(), 19);

    TS_ASSERT_EQUALS(namesList[0].wsIdx, 151);
    TS_ASSERT_EQUALS(namesList[0].spectrumNum, -1);
    TS_ASSERT_DELTA(namesList[0].numericValue, -0.01, 1e-3);
    TS_ASSERT(namesList[0].ws);
    TS_ASSERT_EQUALS(namesList[0].name, "saveNISTDAT_data.nxs");

    TS_ASSERT_EQUALS(namesList[18].wsIdx, 169);
    TS_ASSERT_EQUALS(namesList[18].spectrumNum, -1);
    TS_ASSERT_DELTA(namesList[18].numericValue, 0.01, 1e-3);
    TS_ASSERT(namesList[18].ws);
    TS_ASSERT_EQUALS(namesList[18].name, "saveNISTDAT_data.nxs");
  }

  void testWorkspaceRangeSpecifiedNumericAxisOutOfBounds() {
    std::string workspaceName = "saveNISTDAT_data.nxs,v-0.01:100";

    auto namesList = makeNames(workspaceName, -1, -1);
    auto inputWithWorkspace = namesList[0];

    TS_ASSERT_EQUALS(namesList.size(), 1);
    TS_ASSERT_EQUALS(inputWithWorkspace.wsIdx, -1);
    TS_ASSERT_EQUALS(inputWithWorkspace.spectrumNum, -1);
    TS_ASSERT_EQUALS(inputWithWorkspace.numericValue, -1);
    TS_ASSERT(!inputWithWorkspace.ws);
    TS_ASSERT_EQUALS(inputWithWorkspace.name, "");
  }
};
