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
#include "MantidPythonInterface/core/WrapPython.h"
#include "MantidQtWidgets/Plotting/QtPlot.h"

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace {

MatrixWorkspace_sptr createMatrixWorkspace(int numberOfHistograms, int numberOfBoundaries = 4) {
  return WorkspaceCreationHelper::create2DWorkspace(numberOfHistograms, numberOfBoundaries);
}

} // namespace

/// Unit tests for QtPlot
class QtPlotTest : public CxxTest::TestSuite {
public:
  static QtPlotTest *createSuite() { return new QtPlotTest(); }

  static void destroySuite(QtPlotTest *suite) { delete suite; }

  QtPlotTest() { PyImport_ImportModule("mantid.plots"); }

  void test_constructor() { TS_ASSERT_THROWS_NOTHING(QtPlot(nullptr)); }

  void test_add_spectrum() {
    auto plot = QtPlot(nullptr);
    auto ws = createMatrixWorkspace(3);

    TS_ASSERT_THROWS_NOTHING(plot.addSpectrum(ws, 1));
  }
};
