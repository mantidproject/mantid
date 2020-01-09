// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PLOTPEAKBYLOGVALUEHELPERTEST_H_
#define PLOTPEAKBYLOGVALUEHELPERTEST_H_

#include "MantidAPI/AlgorithmManager.h"
//#include "MantidAPI/FileFinder.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidCurveFitting/Algorithms/PlotPeakByLogValueHelper.h"
#include <cxxtest/TestSuite.h>
#include <iostream>

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
    auto alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged("Load");
    alg->setChild(true);
    alg->initialize();
    alg->setProperty("OutputWorkspace", "__temp_workspace");

    auto namesList = makeNames(workspaceName, 0, -1);
    auto inputWithWorkspace = getWorkspace(namesList[0], alg);

    TS_ASSERT_EQUALS(inputWithWorkspace.i, 0);
    TS_ASSERT_EQUALS(inputWithWorkspace.spec, 1);
    TS_ASSERT(inputWithWorkspace.ws);
    TS_ASSERT_EQUALS(inputWithWorkspace.name, "irs26176_graphite002_red.nxs");
    TS_ASSERT(inputWithWorkspace.indx.empty());
  }

  void testWorkspaceSpectrumSpecified() {
    std::string workspaceName = "irs26176_graphite002_red.nxs,sp1";
    auto alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged("Load");
    alg->setChild(true);
    alg->initialize();
    alg->setProperty("OutputWorkspace", "__temp_workspace");

    auto namesList = makeNames(workspaceName, -1, -1);
    auto inputWithWorkspace = getWorkspace(namesList[0], alg);

    TS_ASSERT_EQUALS(inputWithWorkspace.i, 0);
    TS_ASSERT_EQUALS(inputWithWorkspace.spec, 1);
    TS_ASSERT(inputWithWorkspace.ws);
    TS_ASSERT_EQUALS(inputWithWorkspace.name, "irs26176_graphite002_red.nxs");
    TS_ASSERT(inputWithWorkspace.indx.empty());
  }

  void testWorkspaceRangeSpecifiedSpectrumAxis() {
    std::string workspaceName = "irs26176_graphite002_red.nxs,v1.1:3.2";
    auto alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged("Load");
    alg->setChild(true);
    alg->initialize();
    alg->setProperty("OutputWorkspace", "__temp_workspace");

    auto namesList = makeNames(workspaceName, -1, -1);
    auto inputWithWorkspace = getWorkspace(namesList[0], alg);

    TS_ASSERT_EQUALS(inputWithWorkspace.i, -1);
    TS_ASSERT_EQUALS(inputWithWorkspace.spec, -1);
    TS_ASSERT(inputWithWorkspace.ws);
    TS_ASSERT_EQUALS(inputWithWorkspace.name, "irs26176_graphite002_red.nxs");
    TS_ASSERT_EQUALS(inputWithWorkspace.indx, std::vector<int>({1, 2}));
  }

  void testWorkspaceIndexNumericAxis() {
    std::string workspaceName = "saveNISTDAT_data.nxs,i0";
    auto alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged("Load");
    alg->setChild(true);
    alg->initialize();
    alg->setProperty("OutputWorkspace", "__temp_workspace");

    auto namesList = makeNames(workspaceName, -1, -1);
    auto inputWithWorkspace = getWorkspace(namesList[0], alg);

    TS_ASSERT_EQUALS(inputWithWorkspace.i, 0);
    TS_ASSERT_EQUALS(inputWithWorkspace.spec, -1);
    TS_ASSERT(inputWithWorkspace.ws);
    TS_ASSERT_EQUALS(inputWithWorkspace.name, "saveNISTDAT_data.nxs");
    TS_ASSERT_EQUALS(inputWithWorkspace.indx, std::vector<int>({}));
  }

  void testWorkspaceSpectrumNumericAxis() {
    std::string workspaceName = "saveNISTDAT_data.nxs,sp1";
    auto alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged("Load");
    alg->setChild(true);
    alg->initialize();
    alg->setProperty("OutputWorkspace", "__temp_workspace");

    auto namesList = makeNames(workspaceName, -1, -1);
    auto inputWithWorkspace = getWorkspace(namesList[0], alg);

    TS_ASSERT_EQUALS(inputWithWorkspace.i, -1);
    TS_ASSERT_EQUALS(inputWithWorkspace.spec, -1);
    TS_ASSERT(inputWithWorkspace.ws);
    TS_ASSERT_EQUALS(inputWithWorkspace.name, "saveNISTDAT_data.nxs");
    TS_ASSERT_EQUALS(inputWithWorkspace.indx, std::vector<int>({160}));
  }

  void testWorkspaceRangeSpecifiedNumericAxisAll() {
    std::string workspaceName = "saveNISTDAT_data.nxs";
    auto alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged("Load");
    alg->setChild(true);
    alg->initialize();
    alg->setProperty("OutputWorkspace", "__temp_workspace");

    auto namesList = makeNames(workspaceName, -2, -2);
    auto inputWithWorkspace = getWorkspace(namesList[0], alg);

    TS_ASSERT_EQUALS(inputWithWorkspace.i, -2);
    TS_ASSERT_EQUALS(inputWithWorkspace.spec, -1);
    TS_ASSERT(inputWithWorkspace.ws);
    TS_ASSERT_EQUALS(inputWithWorkspace.name, "saveNISTDAT_data.nxs");
    TS_ASSERT_EQUALS(inputWithWorkspace.indx.size(), 321);
  }

  void testWorkspaceRangeSpecifiedNumericAxis() {
    std::string workspaceName = "saveNISTDAT_data.nxs,v-0.01:0.01";
    auto alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged("Load");
    alg->setChild(true);
    alg->initialize();
    alg->setProperty("OutputWorkspace", "__temp_workspace");

    auto namesList = makeNames(workspaceName, -1, -1);
    auto inputWithWorkspace = getWorkspace(namesList[0], alg);

    TS_ASSERT_EQUALS(inputWithWorkspace.i, -1);
    TS_ASSERT_EQUALS(inputWithWorkspace.spec, -1);
    TS_ASSERT_EQUALS(inputWithWorkspace.start, -0.01);
    TS_ASSERT_EQUALS(inputWithWorkspace.end, 0.01);
    TS_ASSERT(inputWithWorkspace.ws);
    TS_ASSERT_EQUALS(inputWithWorkspace.name, "saveNISTDAT_data.nxs");
    TS_ASSERT_EQUALS(
        inputWithWorkspace.indx,
        std::vector<int>({151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161,
                          162, 163, 164, 165, 166, 167, 168, 169}));
  }
};

#endif /*PLOTPEAKBYLOGVALUEHELPERTEST_H_*/