// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectDataAnalysisTab.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"
#include "MantidAPI/FunctionFactory.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using namespace MantidQt::CustomInterfaces::IDA;

class IndirectDataAnalysisTabTest : public CxxTest::TestSuite {
public:
  /// Needed to make sure everything is initialized
  IndirectDataAnalysisTabTest() = default;

  static IndirectDataAnalysisTabTest *createSuite() {
    return new IndirectDataAnalysisTabTest();
  }

  static void destroySuite(IndirectDataAnalysisTabTest *suite) {
    delete suite;
  }

  void test_updatePlot() { 
    const std::string wsName = "wsName";
    size_t index = 1;
  }

  //MantidQt::MantidWidgets::PreviewPlot m_fitPreviewPlot;
  //MantidQt::MantidWidgets::PreviewPlot m_diffPreviewPlot;

};
  