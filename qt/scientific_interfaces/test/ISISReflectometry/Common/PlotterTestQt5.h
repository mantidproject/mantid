// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "../../../ISISReflectometry/GUI/Common/Plotter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ConfigService.h"
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
    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
        "CreateSampleWorkspace");
    alg->initialize();
    alg->setProperty("OutputWorkspace", "ws1");
    alg->execute();

    MantidQt::CustomInterfaces::ISISReflectometry::Plotter plotter;
    plotter.reflectometryPlot({"ws1"});
  }
};
