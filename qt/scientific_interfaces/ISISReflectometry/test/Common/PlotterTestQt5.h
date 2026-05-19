// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Common/Plotter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidPythonInterface/core/WrapPython.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include <boost/python/extract.hpp>
#include <cxxtest/TestSuite.h>

class PlotterTestQt5 : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PlotterTestQt5 *createSuite() { return new PlotterTestQt5(); }
  static void destroySuite(PlotterTestQt5 *suite) { delete suite; }

  PlotterTestQt5() { Mantid::API::FrameworkManager::Instance(); }

  void tearDown() override { closeAllFigures(); }

  void testReflectometryPlot() {
    // Just test that it doesn't segfault when plotting as nothing is returned
    // or accessible from here to test
    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("CreateSampleWorkspace");
    alg->initialize();
    alg->setProperty("OutputWorkspace", "ws1");
    alg->execute();

    MantidQt::CustomInterfaces::ISISReflectometry::Plotter plotter;
    plotter.plot({{"ws1"},
                  MantidQt::CustomInterfaces::ISISReflectometry::reflectivityCurvePlotOptions(
                      MantidQt::CustomInterfaces::ISISReflectometry::PlotOutputType::ReflectivityCurve,
                      MantidQt::CustomInterfaces::ISISReflectometry::PlotLayout::Individual)});
  }

  void testReflectometryOverplotCreatesNewFigure() {
    closeAllFigures();
    createWorkspace("ws1");
    createWorkspace("ws2");

    MantidQt::CustomInterfaces::ISISReflectometry::Plotter plotter;
    plotter.plot({{"ws1"},
                  MantidQt::CustomInterfaces::ISISReflectometry::reflectivityCurvePlotOptions(
                      MantidQt::CustomInterfaces::ISISReflectometry::PlotOutputType::ReflectivityCurve,
                      MantidQt::CustomInterfaces::ISISReflectometry::PlotLayout::Individual)});
    TS_ASSERT_EQUALS(figureCount(), 1);

    plotter.plot({{"ws1", "ws2"},
                  MantidQt::CustomInterfaces::ISISReflectometry::reflectivityCurvePlotOptions(
                      MantidQt::CustomInterfaces::ISISReflectometry::PlotOutputType::ReflectivityCurve,
                      MantidQt::CustomInterfaces::ISISReflectometry::PlotLayout::Overplot)});

    TS_ASSERT_EQUALS(figureCount(), 2);
  }

  void testSpinAsymmetryPlotUsesConfiguredYAxisLabel() {
    closeAllFigures();
    createWorkspace("ws1");

    MantidQt::CustomInterfaces::ISISReflectometry::Plotter plotter;
    plotter.plot({{"ws1"},
                  MantidQt::CustomInterfaces::ISISReflectometry::spinAsymmetryPlotOptions(
                      MantidQt::CustomInterfaces::ISISReflectometry::PlotLayout::Individual)});

    TS_ASSERT_EQUALS(currentFigureYAxisLabel(), "Spin Asymmetry");
  }

  void testDetectorMapPlotUsesConfiguredAxisLabels() {
    closeAllFigures();
    createWorkspace("ws1");

    MantidQt::CustomInterfaces::ISISReflectometry::Plotter plotter;
    plotter.plot({{"ws1"},
                  MantidQt::CustomInterfaces::ISISReflectometry::detectorMapPlotOptions(
                      MantidQt::CustomInterfaces::ISISReflectometry::DetectorMapXAxis::Lambda,
                      MantidQt::CustomInterfaces::ISISReflectometry::DetectorMapYAxis::Theta,
                      MantidQt::CustomInterfaces::ISISReflectometry::PlotLayout::Individual)});

    TS_ASSERT_EQUALS(currentFigureXAxisLabel(), "Lambda");
    TS_ASSERT_EQUALS(currentFigureYAxisLabel(), "Theta");
    TS_ASSERT_EQUALS(currentFigureColorbarLabel(), "Intensity");
  }

  void testTiledPlotKeepsWorkspaceGroupMembersOnSameAxis() {
    closeAllFigures();
    createWorkspaceGroup("alignment_1", {"alignment_1_raw", "alignment_1_calc", "alignment_1_peak"});
    createWorkspaceGroup("alignment_2", {"alignment_2_raw", "alignment_2_calc", "alignment_2_peak"});
    createWorkspaceGroup("alignment_3", {"alignment_3_raw", "alignment_3_calc", "alignment_3_peak"});

    MantidQt::CustomInterfaces::ISISReflectometry::Plotter plotter;
    plotter.plot({{"alignment_1", "alignment_2", "alignment_3"},
                  MantidQt::CustomInterfaces::ISISReflectometry::alignmentPlotOptions(
                      MantidQt::CustomInterfaces::ISISReflectometry::AlignmentXAxis::DetectorId,
                      MantidQt::CustomInterfaces::ISISReflectometry::PlotLayout::Tiled)});

    auto const lineCounts = populatedAxisLineCounts();
    TS_ASSERT_EQUALS(lineCounts.size(), 3);
    for (auto const lineCount : lineCounts) {
      TS_ASSERT_EQUALS(lineCount, 3);
    }
  }

private:
  void createWorkspace(std::string const &name) {
    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("CreateSampleWorkspace");
    alg->initialize();
    alg->setProperty("OutputWorkspace", name);
    alg->execute();
  }

  void createWorkspaceGroup(std::string const &groupName, std::vector<std::string> const &workspaceNames) {
    auto inputWorkspaces = std::string{};
    for (auto const &workspaceName : workspaceNames) {
      createWorkspace(workspaceName);
      if (!inputWorkspaces.empty()) {
        inputWorkspaces += ",";
      }
      inputWorkspaces += workspaceName;
    }
    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("GroupWorkspaces");
    alg->initialize();
    alg->setProperty("InputWorkspaces", inputWorkspaces);
    alg->setProperty("OutputWorkspace", groupName);
    alg->execute();
  }

  int figureCount() {
    Mantid::PythonInterface::GlobalInterpreterLock lock;
    MantidQt::Widgets::Common::Python::Object pyplot{
        MantidQt::Widgets::Common::Python::NewRef(PyImport_ImportModule("matplotlib.pyplot"))};
    auto const figureNumbers = pyplot.attr("get_fignums")();
    return static_cast<int>(PySequence_Size(figureNumbers.ptr()));
  }

  std::string currentFigureYAxisLabel() { return currentFigureAxisLabel(0, "get_ylabel"); }

  std::string currentFigureXAxisLabel() { return currentFigureAxisLabel(0, "get_xlabel"); }

  std::string currentFigureColorbarLabel() { return currentFigureAxisLabel(1, "get_ylabel"); }

  std::string currentFigureAxisLabel(int const axisIndex, char const *labelGetter) {
    Mantid::PythonInterface::GlobalInterpreterLock lock;
    MantidQt::Widgets::Common::Python::Object pyplot{
        MantidQt::Widgets::Common::Python::NewRef(PyImport_ImportModule("matplotlib.pyplot"))};
    auto const figure = MantidQt::Widgets::Common::Python::Object(pyplot.attr("gcf")());
    auto const axes = figure.attr("axes");
    auto const axis = MantidQt::Widgets::Common::Python::Object(axes[axisIndex]);
    return boost::python::extract<std::string>(axis.attr(labelGetter)());
  }

  std::vector<int> populatedAxisLineCounts() {
    Mantid::PythonInterface::GlobalInterpreterLock lock;
    MantidQt::Widgets::Common::Python::Object pyplot{
        MantidQt::Widgets::Common::Python::NewRef(PyImport_ImportModule("matplotlib.pyplot"))};
    auto const figure = MantidQt::Widgets::Common::Python::Object(pyplot.attr("gcf")());
    auto const axes = figure.attr("axes");
    auto const axesCount = MantidQt::Widgets::Common::Python::Len(axes);
    auto lineCounts = std::vector<int>{};
    for (auto index = 0; index < axesCount; ++index) {
      auto const axis = MantidQt::Widgets::Common::Python::Object(axes[index]);
      auto const lineCount = static_cast<int>(MantidQt::Widgets::Common::Python::Len(axis.attr("get_lines")()));
      if (lineCount > 0) {
        lineCounts.emplace_back(lineCount);
      }
    }
    return lineCounts;
  }

  void closeAllFigures() {
    Mantid::PythonInterface::GlobalInterpreterLock lock;
    MantidQt::Widgets::Common::Python::Object pyplot{
        MantidQt::Widgets::Common::Python::NewRef(PyImport_ImportModule("matplotlib.pyplot"))};
    pyplot.attr("close")("all");
  }
};
