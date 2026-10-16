// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/CalculateLogLikelihoodEvidence.h"
#include "MantidDataObjects/Workspace2D.h"
#include <cmath>
#include <cxxtest/TestSuite.h>

using Mantid::API::AnalysisDataService;
using Mantid::API::ITableWorkspace;
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_sptr;
using Mantid::API::WorkspaceFactory;
using Mantid::API::WorkspaceGroup;
using Mantid::API::WorkspaceProperty;

class CalculateLogLikelihoodEvidenceTest : public CxxTest::TestSuite {
public:
  static CalculateLogLikelihoodEvidenceTest *createSuite() { return new CalculateLogLikelihoodEvidenceTest(); }
  static void destroySuite(CalculateLogLikelihoodEvidenceTest *suite) { delete suite; }

  void test_name() {
    Mantid::Algorithms::CalculateLogLikelihoodEvidence alg;
    TS_ASSERT_EQUALS(alg.name(), "CalculateLogLikelihoodEvidence");
  }

  void test_version() {
    Mantid::Algorithms::CalculateLogLikelihoodEvidence alg;
    TS_ASSERT_EQUALS(alg.version(), 1);
  }

  void test_init_declares_expected_properties() {

    Mantid::Algorithms::CalculateLogLikelihoodEvidence alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    const auto &props = alg.getProperties();
    TS_ASSERT_EQUALS(props.size(), 3);

    TS_ASSERT_EQUALS(props[0]->name(), "WorkspaceList");
    TS_ASSERT(props[0]->isDefault());

    TS_ASSERT_EQUALS(props[1]->name(), "OutputWorkspace");
    TS_ASSERT(props[1]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<ITableWorkspace> *>(props[1]));

    TS_ASSERT_EQUALS(props[2]->name(), "OutputRelativeFactors");
    TS_ASSERT(props[2]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<WorkspaceGroup> *>(props[2]));
  }

  void test_execute_calculates_evidence_for_simple_workspace() {

    const auto inputName = "CalculateLogLikelihoodEvidenceTestInput";
    const auto outputName = "CalculateLogLikelihoodEvidenceTestOutput";
    const auto relativeFactorsName = "CalculateLogLikelihoodEvidenceTestRelativeFactors";

    auto cleanup = [&]() {
      AnalysisDataService::Instance().remove(inputName);
      AnalysisDataService::Instance().remove(outputName);
      AnalysisDataService::Instance().remove(relativeFactorsName);
    };
    cleanup();

    MatrixWorkspace_sptr inputWorkspace =
        std::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceFactory::Instance().create("Workspace2D", 1, 2, 1));
    auto &x = inputWorkspace->mutableX(0);
    auto &y = inputWorkspace->mutableY(0);
    x[0] = 0.0;
    x[1] = 1.0;
    y[0] = 1.0;

    AnalysisDataService::Instance().addOrReplace(inputName, inputWorkspace);

    Mantid::Algorithms::CalculateLogLikelihoodEvidence alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    alg.setProperty("WorkspaceList", std::vector<std::string>{inputName});
    alg.setProperty("OutputWorkspace", outputName);
    alg.setProperty("OutputRelativeFactors", relativeFactorsName);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    auto outputWorkspace = AnalysisDataService::Instance().retrieveWS<Mantid::API::ITableWorkspace>(outputName);
    TS_ASSERT(outputWorkspace);
    TS_ASSERT_EQUALS(outputWorkspace->rowCount(), 1);
    TS_ASSERT_DELTA(outputWorkspace->cell<double>(0, 1), -0.25, 1e-12);

    auto relativeFactorsWorkspace = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(relativeFactorsName);
    TS_ASSERT(relativeFactorsWorkspace);

    cleanup();
  }

  void test_calculate_log_likelihood_evidence_for_uniform_positive_pdf() {
    using Mantid::API::MatrixWorkspace;
    using Mantid::API::WorkspaceFactory;

    MatrixWorkspace_sptr inputWorkspace =
        std::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceFactory::Instance().create("Workspace2D", 1, 6, 5));
    auto &x = inputWorkspace->mutableX(0);
    auto &y = inputWorkspace->mutableY(0);

    std::vector<double> xValues{0.0, 1.0, 2.0, 3.0, 4.0, 5.0};
    std::vector<double> yValues{1.0, 1.0, 1.0, 1.0, 1.0};

    for (size_t i = 0; i < xValues.size(); ++i) {
      x[i] = xValues[i];
    }
    for (size_t i = 0; i < yValues.size(); ++i) {
      y[i] = yValues[i];
    }

    Mantid::Algorithms::CalculateLogLikelihoodEvidence alg;
    const double evidence = alg.calculateLogLikelihoodEvidence(inputWorkspace);

    TS_ASSERT_DELTA(evidence, 0.5971016458251504, 1e-12);
  }

  void test_calculate_log_likelihood_evidence_for_non_uniform_positive_pdf() {
    using Mantid::API::MatrixWorkspace;
    using Mantid::API::WorkspaceFactory;

    MatrixWorkspace_sptr inputWorkspace =
        std::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceFactory::Instance().create("Workspace2D", 1, 6, 5));
    auto &x = inputWorkspace->mutableX(0);
    auto &y = inputWorkspace->mutableY(0);

    std::vector<double> xValues{0.0, 1.0, 2.0, 3.0, 4.0, 5.0};
    std::vector<double> yValues{1.0, 1.5, 2.0, 1.5, 1.0};

    for (size_t i = 0; i < xValues.size(); ++i) {
      x[i] = xValues[i];
    }
    for (size_t i = 0; i < yValues.size(); ++i) {
      y[i] = yValues[i];
    }

    Mantid::Algorithms::CalculateLogLikelihoodEvidence alg;
    const double evidence = alg.calculateLogLikelihoodEvidence(inputWorkspace);

    TS_ASSERT_DELTA(evidence, 0.8864170624711742, 1e-12);
  }

  void test_calculate_log_likelihood_evidence_for_non_uniform_positive_pdf_large_bin_width() {
    using Mantid::API::MatrixWorkspace;
    using Mantid::API::WorkspaceFactory;

    MatrixWorkspace_sptr inputWorkspace =
        std::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceFactory::Instance().create("Workspace2D", 1, 6, 5));
    auto &x = inputWorkspace->mutableX(0);
    auto &y = inputWorkspace->mutableY(0);

    std::vector<double> xValues{200.0, 260.0, 320.0, 380.0, 440.0, 500.0};
    std::vector<double> yValues{1.0, 1.5, 2.0, 1.5, 1.0};

    for (size_t i = 0; i < xValues.size(); ++i) {
      x[i] = xValues[i];
    }
    for (size_t i = 0; i < yValues.size(); ++i) {
      y[i] = yValues[i];
    }

    Mantid::Algorithms::CalculateLogLikelihoodEvidence alg;
    const double evidence = alg.calculateLogLikelihoodEvidence(inputWorkspace);

    TS_ASSERT_DELTA(evidence, -110.90565543777775, 1e-12);
  }
};
