// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "../../../ISISReflectometry/GUI/Plotting/Plotter.h"
#include "../ReflMockObjects.h"
#include <cxxtest/TestSuite.h>

class PlotterTestQt4 : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PlotterTestQt4 *createSuite() { return new PlotterTestQt4(); }
  static void destroySuite(PlotterTestQt4 *suite) { delete suite; }

  void testReflectometryPlot() {
    MockMainWindowView mainWindow;
    auto plotter = Plotter(&mainWindow);
    const std::string testCode =
        "base_graph = None\nbase_graph = plotSpectrum(\"ws1\", 0, True, window "
        "= base_graph)\nbase_graph.activeLayer().logLogAxes()\n";

    EXPECT_CALL(mainWindow, runPythonAlgorithm(testCode));

    plotter.reflectometryPlot({"ws1"});
  }

  void testRunPythonCode() {
    MockMainWindowView mainWindow;
    auto plotter = Plotter(&mainWindow);
    const std::string testCode = "test code";

    EXPECT_CALL(mainWindow, runPythonAlgorithm(testCode));

    plotter.runPython(testCode);
  }
};