// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "../../../ISISReflectometry/GUI/Plotting/Plotter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include <cxxtest/TestSuite.h>

namespace {
void setMatplotlibBackend() {
  // Setup python and mantid python
  Py_Initialize();
  const MantidQt::Widgets::Common::Python::Object siteModule{
      MantidQt::Widgets::Common::Python::NewRef(PyImport_ImportModule("site"))};
  siteModule.attr("addsitedir")(
      Mantid::Kernel::ConfigService::Instance().getPropertiesDir());

  // Set matplotlib backend
  auto mpl = MantidQt::Widgets::Common::Python::NewRef(
      PyImport_ImportModule("matplotlib"));
  mpl.attr("use")("Agg");
}
} // namespace

class PlotterTestQt5 : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PlotterTestQt5 *createSuite() { return new PlotterTestQt5(); }
  static void destroySuite(PlotterTestQt5 *suite) { delete suite; }

  void setUp() override {
    Mantid::API::FrameworkManager::Instance();
    setMatplotlibBackend();
  }

  void tearDown() override { Py_Finalize(); }

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