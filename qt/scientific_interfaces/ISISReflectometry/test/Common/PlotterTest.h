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
#include "MantidQtWidgets/Common/Python/Object.h"
#include <cxxtest/TestSuite.h>

class PlotterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PlotterTest *createSuite() { return new PlotterTest(); }
  static void destroySuite(PlotterTest *suite) { delete suite; }

  PlotterTest() { Mantid::API::FrameworkManager::Instance(); }

  void testReflectometryPlot() {
    // Just test that it doesn't segfault when plotting as nothing is returned
    // or accessible from here to test
    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("CreateSampleWorkspace");
    alg->initialize();
    alg->setProperty("OutputWorkspace", "ws1");
    alg->execute();

    MantidQt::CustomInterfaces::ISISReflectometry::Plotter plotter;
    plotter.reflectometryPlot({"ws1"});
  }
};
