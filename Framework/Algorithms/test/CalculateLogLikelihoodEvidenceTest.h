// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/CalculateLogLikelihoodEvidence.h"

#include <cxxtest/TestSuite.h>

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
    using Mantid::API::ITableWorkspace;
    using Mantid::API::WorkspaceGroup;
    using Mantid::API::WorkspaceProperty;

    Mantid::Algorithms::CalculateLogLikelihoodEvidence alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    const auto &props = alg.getProperties();
    TS_ASSERT_EQUALS(props.size(), 3);

    TS_ASSERT_EQUALS(props[0]->name(), "WorkspaceList");
    TS_ASSERT(props[0]->isDefault());

    TS_ASSERT_EQUALS(props[1]->name(), "LogLikelihoodEvidence");
    TS_ASSERT(props[1]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<ITableWorkspace> *>(props[1]));

    TS_ASSERT_EQUALS(props[2]->name(), "RelativeFactors");
    TS_ASSERT(props[2]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<WorkspaceGroup> *>(props[2]));
  }
};
