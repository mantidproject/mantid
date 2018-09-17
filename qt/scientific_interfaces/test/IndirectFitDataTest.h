#ifndef MANTID_INDIRECTFITDATATEST_H_
#define MANTID_INDIRECTFITDATATEST_H_

#include <cxxtest/TestSuite.h>

#include "Indirect/IndirectFitData.h"
//#include "Indirect/IndirectFitData.cpp"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"

#include <iostream>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces::IDA;

class IndirectFitDataTest : public CxxTest::TestSuite {
public:
  static IndirectFitDataTest *createSuite() {
    return new IndirectFitDataTest();
  }

  static void destroySuite(IndirectFitDataTest *suite) { delete suite; }

  void test_data_is_instantiated_correctly() {
    MatrixWorkspace_sptr workspace =
        WorkspaceFactory::Instance().create("Workspace2D", 1, 9, 9);
    const Spectra spec =
        std::make_pair(0u, workspace->getNumberHistograms() - 1);
    IndirectFitData data(workspace, spec);

    const std::string name = data.workspace()->getName();
    std::cout << name << "\n";
  }

  void test_displayName_returns_correct_name() {
  //given -
  //the workspace and data
  //when -
  //the parameters equal so and so
  //then -
  //the output name should be
  }
};

#endif
