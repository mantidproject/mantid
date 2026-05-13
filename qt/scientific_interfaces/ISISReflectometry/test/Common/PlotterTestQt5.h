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
#include <cxxtest/TestSuite.h>

class PlotterTestQt5 : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PlotterTestQt5 *createSuite() { return new PlotterTestQt5(); }
  static void destroySuite(PlotterTestQt5 *suite) { delete suite; }

  PlotterTestQt5() { Mantid::API::FrameworkManager::Instance(); }

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
    closeAllFigures();
  }

private:
  void createWorkspace(std::string const &name) {
    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("CreateSampleWorkspace");
    alg->initialize();
    alg->setProperty("OutputWorkspace", name);
    alg->execute();
  }

  int figureCount() {
    Mantid::PythonInterface::GlobalInterpreterLock lock;
    MantidQt::Widgets::Common::Python::Object pyplot{
        MantidQt::Widgets::Common::Python::NewRef(PyImport_ImportModule("matplotlib.pyplot"))};
    auto const figureNumbers = pyplot.attr("get_fignums")();
    return static_cast<int>(PySequence_Size(figureNumbers.ptr()));
  }

  void closeAllFigures() {
    Mantid::PythonInterface::GlobalInterpreterLock lock;
    MantidQt::Widgets::Common::Python::Object pyplot{
        MantidQt::Widgets::Common::Python::NewRef(PyImport_ImportModule("matplotlib.pyplot"))};
    pyplot.attr("close")("all");
  }
};
