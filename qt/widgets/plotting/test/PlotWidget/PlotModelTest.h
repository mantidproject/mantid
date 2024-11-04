// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidQtWidgets/Plotting/PlotWidget/PlotModel.h"

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace {

MatrixWorkspace_sptr createMatrixWorkspace(int numberOfHistograms, int numberOfBoundaries = 4) {
  return WorkspaceCreationHelper::create2DWorkspace(numberOfHistograms, numberOfBoundaries);
}

} // namespace

/// Unit tests for PlotModel
class PlotModelTest : public CxxTest::TestSuite {
public:
  static PlotModelTest *createSuite() { return new PlotModelTest(); }

  static void destroySuite(PlotModelTest *suite) { delete suite; }

  void test_set_spectrum() {
    auto model = PlotModel();
    auto ws = createMatrixWorkspace(3);

    model.setSpectrum(ws, 1);

    std::vector<int> expectedIndices{1};
    std::vector<Mantid::API::MatrixWorkspace_sptr> expectedWorkspaces{ws};

    TS_ASSERT_EQUALS(model.getWorkspaceIndices(), expectedIndices);
    TS_ASSERT_EQUALS(model.getWorkspaces(), expectedWorkspaces);
  }

  void test_clear_will_clear_the_model() {
    auto model = PlotModel();
    auto ws = createMatrixWorkspace(3);

    model.setSpectrum(ws, 1);

    model.clear();

    TS_ASSERT(model.getWorkspaceIndices().empty());
    TS_ASSERT(model.getWorkspaces().empty());
  }

  void test_set_get_plot_error_bars() {
    auto model = PlotModel();
    model.setPlotErrorBars(true);

    TS_ASSERT(model.getPlotErrorBars());
  }
};
