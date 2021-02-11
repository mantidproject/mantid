// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidCurveFitting/Algorithms/ProfileChiSquared1D.h"

class ProfileChiSquared1DTest : public CxxTest::TestSuite {
public:
  static ProfileChiSquared1DTest *createSuite() {
    return new ProfileChiSquared1DTest();
  }
  static void destroySuite(ProfileChiSquared1DTest *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  void test_Init() {
    ProfileChiSquared1D alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_errors_for_linear_function_are_correct() {
    auto algo = AlgorithmManager::Instance().create("Load");
    algo->setPropertyValue("Filename", "ProfileChiSquared1DData_linear.nxs");
    algo->setPropertyValue("OutputWorkspace", "ProfileChiSquared1DData_linear");
    algo->execute();
    auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "ProfileChiSquared1DData_linear");
    auto initialFunString =
        "composite=Convolution,NumDeriv=true,FixResolution=true;name="
        "Resolution,"
        "Workspace=iris26173_graphite002_res,WorkspaceIndex=0,X=(),Y=();name="
        "Lorentzian,Amplitude=1,PeakCentre=0,FWHM=1,constraints=(0<Amplitude,0<"
        "FWHM)";
  }

  void test_pdf_values_for_linear_function_are_correct() {}

  void test_errors_table_has_correct_outputs() {}

  void test_pdf_table_has_correct_outputs(){}
}