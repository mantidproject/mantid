#ifndef DIFFRACTIONFOCUSSINGTEST_H_
#define DIFFRACTIONFOCUSSINGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/DiffractionFocussing.h"
#include "MantidDataHandling/LoadNexus.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;

class DiffractionFocussingTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(focus.name(), "DiffractionFocussing"); }

  void testVersion() { TS_ASSERT_EQUALS(focus.version(), 1); }

  void testInit() {
    focus.initialize();
    TS_ASSERT(focus.isInitialized());
  }

  void testExec() {
    IAlgorithm *loader = new Mantid::DataHandling::LoadNexus;
    loader->initialize();
    loader->setPropertyValue(
        "Filename", "HRP38692a.nxs"); // HRP382692.raw spectrum range 320-330

    std::string outputSpace = "tofocus";
    loader->setPropertyValue("OutputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING(loader->execute());
    TS_ASSERT(loader->isExecuted());

    focus.setPropertyValue("InputWorkspace", outputSpace);
    focus.setPropertyValue("OutputWorkspace", "focusedWS");
    focus.setPropertyValue("GroupingFileName", "hrpd_new_072_01.cal");

    TS_ASSERT_THROWS_NOTHING(focus.execute());
    TS_ASSERT(focus.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "focusedWS"));

    // only 1 group for this limited range of spectra
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 1);

    AnalysisDataService::Instance().remove(outputSpace);
    AnalysisDataService::Instance().remove("focusedWS");
    delete loader;
  }

private:
  DiffractionFocussing focus;
};

#endif /*DIFFRACTIONFOCUSSINGTEST_H_*/
