// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "../../../ISISReflectometry/GUI/Plotting/Plotter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include <cxxtest/TestSuite.h>

namespace {
void setMatplotlibBackend() {
  auto mpl = MantidQt::Widgets::Common::Python::NewRef(
      PyImport_ImportModule("matplotlib"));
  mpl.attr("use")("Agg");
}
} // namespace

class PlotterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PlotterTest *createSuite() { return new PlotterTest(); }
  static void destroySuite(PlotterTest *suite) { delete suite; }

  void setUp() override {
    Mantid::API::FrameworkManager::Instance();
    setMatplotlibBackend();
  }

  void testReflectometryPlot() {
    // Just test that it doesn't segfault when plotting as nothing is returned
    // or accessible from here to test
    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
        "CreateSampleWorkspace");
    alg->initialize();
    alg->setProperty("OutputWorkspace", "ws1");
    alg->execute();

    MantidQt::CustomInterfaces::Plotter plotter;
    plotter.reflectometryPlot({"ws1"});
  }
};