// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PLOTPEAKBYLOGVALUEHELPERTEST_H_
#define PLOTPEAKBYLOGVALUEHELPERTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidCurveFitting/Algorithms/PlotPeakByLogValueHelper.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::CurveFitting::Algorithms;

class PlotPeakByLogValueHelperTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PlotPeakByLogValueHelperTest *createSuite() {
    return new PlotPeakByLogValueHelperTest();
  }
  static void destroySuite(PlotPeakByLogValueHelperTest *suite) {
    delete suite;
  }

  PlotPeakByLogValueHelperTest() {}

  void testWorkspaceIndexSpecified() {
    std::string workspaceName = "irs26176_graphite002_red.nxs,i0";

    auto namesList = makeNames(workspaceName, 0, -1);
    auto inputWithWorkspace = namesList[0];

    TS_ASSERT_EQUALS(namesList.size(), 1);
    TS_ASSERT_EQUALS(inputWithWorkspace.i, 0);
    TS_ASSERT(inputWithWorkspace.ws);
    TS_ASSERT_EQUALS(inputWithWorkspace.name, "irs26176_graphite002_red.nxs");
  }

  void testWorkspaceSpectrumSpecified() {
    std::string workspaceName = "irs26176_graphite002_red.nxs,sp1";

    auto namesList = makeNames(workspaceName, -1, -1);
    auto inputWithWorkspace = namesList[0];

    TS_ASSERT_EQUALS(namesList.size(), 1);
    TS_ASSERT_EQUALS(inputWithWorkspace.i, 0);
    TS_ASSERT(inputWithWorkspace.ws);
    TS_ASSERT_EQUALS(inputWithWorkspace.name, "irs26176_graphite002_red.nxs");
  }

  void testWorkspaceRangeSpecifiedSpectrumAxis() {
    std::string workspaceName = "irs26176_graphite002_red.nxs,v1.1:3.2";

    auto namesList = makeNames(workspaceName, -1, -1);
    auto inputWithWorkspace = namesList[0];

    TS_ASSERT_EQUALS(namesList.size(), 2);
    TS_ASSERT_EQUALS(inputWithWorkspace.i, 1);
    TS_ASSERT_EQUALS(namesList[1].i, 2);
    TS_ASSERT(inputWithWorkspace.ws);
    TS_ASSERT_EQUALS(inputWithWorkspace.name, "irs26176_graphite002_red.nxs");
  }

  void testWorkspaceIndexNumericAxis() {
    std::string workspaceName = "saveNISTDAT_data.nxs,i0";

    auto namesList = makeNames(workspaceName, -1, -1);
    auto inputWithWorkspace = namesList[0];

    TS_ASSERT_EQUALS(namesList.size(), 1);
    TS_ASSERT_EQUALS(inputWithWorkspace.i, 0);
    TS_ASSERT(inputWithWorkspace.ws);
    TS_ASSERT_EQUALS(inputWithWorkspace.name, "saveNISTDAT_data.nxs");
  }

  void testWorkspaceSpectrumNumericAxis() {
    std::string workspaceName = "saveNISTDAT_data.nxs,sp1";

    auto namesList = makeNames(workspaceName, -1, -1);
    auto inputWithWorkspace = namesList[0];

    TS_ASSERT_EQUALS(namesList.size(), 1);
    TS_ASSERT_EQUALS(inputWithWorkspace.i, 160);
    TS_ASSERT(inputWithWorkspace.ws);
    TS_ASSERT_EQUALS(inputWithWorkspace.name, "saveNISTDAT_data.nxs");
  }

  void testWorkspaceRangeSpecifiedNumericAxisAll() {
    std::string workspaceName = "saveNISTDAT_data.nxs";

    auto namesList = makeNames(workspaceName, -2, -2);
    auto inputWithWorkspace = namesList[0];

    TS_ASSERT_EQUALS(namesList.size(), 321);
    TS_ASSERT_EQUALS(inputWithWorkspace.i, 0);
    TS_ASSERT_EQUALS(namesList[200].i, 200);
    TS_ASSERT(inputWithWorkspace.ws);
    TS_ASSERT_EQUALS(inputWithWorkspace.name, "saveNISTDAT_data.nxs");
  }

  void testWorkspaceRangeSpecifiedNumericAxis() {
    std::string workspaceName = "saveNISTDAT_data.nxs,v-0.01:0.01";

    auto namesList = makeNames(workspaceName, -1, -1);
    auto inputWithWorkspace = namesList[0];

    TS_ASSERT_EQUALS(namesList.size(), 19);
    TS_ASSERT_EQUALS(inputWithWorkspace.i, 151);
    TS_ASSERT_EQUALS(namesList[15].i, 166);
    TS_ASSERT(inputWithWorkspace.ws);
    TS_ASSERT_EQUALS(inputWithWorkspace.name, "saveNISTDAT_data.nxs");
  }
};

#endif /*PLOTPEAKBYLOGVALUEHELPERTEST_H_*/