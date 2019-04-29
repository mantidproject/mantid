// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_API_MANTIDCONTOURPREVIEWPLOTTEST_H_
#define MANTIDQT_API_MANTIDCONTOURPREVIEWPLOTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Plotting/Qwt/ContourPreviewPlot.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace {

MatrixWorkspace_sptr createMatrixWorkspace(int numberOfHistograms,
                                           int numberOfBoundaries = 4) {
  return WorkspaceCreationHelper::create2DWorkspace(numberOfHistograms,
                                                    numberOfBoundaries);
}

} // namespace

/// This QApplication object is required in order to construct the widget
class QApplicationHolder : CxxTest::GlobalFixture {
public:
  bool setUpWorld() override {
    int argc(0);
    char **argv = {};
    m_app = new QApplication(argc, argv);
    return true;
  }

  bool tearDownWorld() override {
    delete m_app;
    return true;
  }

private:
  QApplication *m_app;
};

static QApplicationHolder MAIN_QAPPLICATION;

/// Unit tests for ContourPreviewPlot
class ContourPreviewPlotTest : public CxxTest::TestSuite {
public:
  static ContourPreviewPlotTest *createSuite() {
    return new ContourPreviewPlotTest();
  }

  static void destroySuite(ContourPreviewPlotTest *suite) { delete suite; }

  void setUp() override {
    m_contourPlot = std::make_unique<ContourPreviewPlot>();
  }

  void tearDown() override { m_contourPlot.reset(); }

  void
  test_that_a_ContourPreviewPlot_is_instantiated_without_an_active_workspace() {
    TS_ASSERT(!m_contourPlot->getActiveWorkspace());
  }

  void test_that_getPlot2D_will_get_the_contour_plot() {
    TS_ASSERT(m_contourPlot->getPlot2D());
  }

  void
  test_that_setWorkspace_will_set_the_active_workspace_for_the_contour_plot() {
    auto const workspace = createMatrixWorkspace(3);

    m_contourPlot->setWorkspace(workspace);

    TS_ASSERT(m_contourPlot->getActiveWorkspace());
    TS_ASSERT_EQUALS(m_contourPlot->getActiveWorkspace(), workspace);
  }

  void test_that_setPlotVisible_will_hide_the_plot_when_it_is_passed_false() {
    m_contourPlot->setPlotVisible(false);
    TS_ASSERT(!m_contourPlot->isPlotVisible());
  }

  void
  test_that_setColourBarVisible_will_hide_the_colour_bar_when_it_is_passed_false() {
    m_contourPlot->setColourBarVisible(false);
    TS_ASSERT(!m_contourPlot->isColourBarVisible());
  }

private:
  std::unique_ptr<ContourPreviewPlot> m_contourPlot;
};

#endif /* MANTIDQT_API_MANTIDCONTOURPREVIEWPLOTTEST_H_ */
