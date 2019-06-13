// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_FITRESOLUTIONCONVOLVEDMODELTEST_H_
#define MANTID_MDALGORITHMS_FITRESOLUTIONCONVOLVEDMODELTEST_H_

#include "MantidMDAlgorithms/Quantification/FitResolutionConvolvedModel.h"
#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::MDAlgorithms::FitResolutionConvolvedModel;

class FitResolutionConvolvedModelTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FitResolutionConvolvedModelTest *createSuite() {
    return new FitResolutionConvolvedModelTest();
  }
  static void destroySuite(FitResolutionConvolvedModelTest *suite) {
    delete suite;
  }

  FitResolutionConvolvedModelTest()
      : m_inputName("FitResolutionConvolvedModelTest") {}

  void test_Init_Does_Not_Throw() {
    Mantid::API::IAlgorithm_sptr alg;
    TS_ASSERT_THROWS_NOTHING(alg = createAlgorithm());
    TS_ASSERT(alg->isInitialized());
  }

  void test_Algorithm_Does_Not_Allow_Standard_MatrixWorkspaces() {
    using namespace Mantid::API;
    IAlgorithm_sptr alg = createAlgorithm();
    MatrixWorkspace_sptr testMatrixWS =
        WorkspaceCreationHelper::create2DWorkspace(1, 10);
    Mantid::API::AnalysisDataService::Instance().addOrReplace(m_inputName,
                                                              testMatrixWS);

    TS_ASSERT_THROWS(alg->setPropertyValue("InputWorkspace", m_inputName),
                     const std::invalid_argument &);

    Mantid::API::AnalysisDataService::Instance().remove(m_inputName);
  }

private:
  Mantid::API::IAlgorithm_sptr createAlgorithm() {
    auto alg = boost::make_shared<FitResolutionConvolvedModel>();
    alg->initialize();
    return alg;
  }

  std::string m_inputName;
};

#endif /* MANTID_MDALGORITHMS_FITRESOLUTIONCONVOLVEDMODELTEST_H_ */
