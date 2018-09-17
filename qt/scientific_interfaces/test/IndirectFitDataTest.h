#ifndef MANTID_INDIRECTFITDATATEST_H_
#define MANTID_INDIRECTFITDATATEST_H_

#include <cxxtest/TestSuite.h>

#include "Indirect/IndirectFitData.h"
//#include "MantidCurveFitting/Algorithms/QENSFitSequential.h"
//#include "MantidAPI/AlgorithmManager.h"
//#include "MantidAPI/FunctionFactory.h"
//#include "MantidAPI/IFunction.h"
//#include "MantidAPI/MatrixWorkspace.h"
//#include "MantidAPI/WorkspaceFactory.h"

#include <iostream>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces::IDA;

namespace {
void setUpFunction() {
  // MatrixWorkspace_sptr workspace =
  //    WorkspaceFactory::Instance().create("workspace_name_red", 1, 9, 9);
  // Spectra spec = "";
  // const std::string function =
  //    "name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);"
  //    "(composite=Convolution,FixResolution=true,NumDeriv=true;"
  //    "name=Resolution,Workspace=resolution,WorkspaceIndex=0;"
  //    "name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0.0175)";

  // IFunction_sptr ifun =
  // FunctionFactory::Instance().createInitialized(function);

  // IAlgorithm_sptr fit = AlgorithmManager::Instance().createUnmanaged("Fit");
  // fit->initialize();
  // fit->setProperty("Function", ifun);
  // fit->setProperty("InputWorkspace", workspace);
  // fit->setProperty("CreateOutput", true);
  // fit->execute();
}

} // namespace

class IndirectFitDataTest : public CxxTest::TestSuite {
public:
  static IndirectFitDataTest *createSuite() {
    return new IndirectFitDataTest();
  }

  static void destroySuite(IndirectFitDataTest *suite) { delete suite; }

  //void test_algorithm_initializes() {
  //  QENSFitSequential alg;
  //  TS_ASSERT_THROWS_NOTHING(alg.initialize());
  //  TS_ASSERT(alg.isInitialized());
  //}

  void test_test() {

    // IndirectFitData::displayName("%1%_s%2%_Result", "_to_");

    // IndirectFitData data(workspace, spec);
    //setUpFunction();
    // data->displayName();

    std::cout << "hello";
  }
};

#endif
