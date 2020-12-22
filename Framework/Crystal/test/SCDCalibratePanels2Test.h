// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCrystal/SCDCalibratePanels2.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Logger.h"

#include <cxxtest/TestSuite.h>
#include <stdexcept>


using namespace Mantid::API;
using namespace Mantid::Crystal;
using namespace Mantid::DataObjects;

class SCDCalibratePanels2Test : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SCDCalibratePanels2Test *createSuite() { return new SCDCalibratePanels2Test(); }
  static void destroySuite( SCDCalibratePanels2Test *suite ) { delete suite; }

  void testName() {
    SCDCalibratePanels2 alg;
    TS_ASSERT_EQUALS(alg.name(), "SCDCalibratePanels2");
  }

  void testInit(){
    SCDCalibratePanels2 alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
  }

  //TODO: the remaining test needs to be implemented
  // ///TODO: test validators

  ///NULL case
  void testNullCase(){
    SCDCalibratePanels2 alg;

    const std::string wsname("ws_nullcase");
    generateSimulatedworkspace(wsname);

    EventWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsname);
  }

  ///Adjust T0 and L1
  void testGlobalShiftOnly(){
    SCDCalibratePanels2 alg;
    const std::string wsname("ws_changeL1");

    //TODO:
  }

  ///Ideal global with one panels moved
  void testSinglePanelMoved(){
    SCDCalibratePanels2 alg;
    const std::string wsname("ws_moveBank");
  }

  ///Ideal global with two panels moved
  void testDualPanelMoved(){
    SCDCalibratePanels2 alg;
    const std::string wsname("ws_moveBanks");
  }

  // Test with mocked CORELLI instrument
  // T0, L1 adjusted
  // Two panels moved
  void testExec(){
    SCDCalibratePanels2 alg;
    const std::string wsname("ws_moveAll");
  }

private:
  // lattice constants of silicon
  const double silicon_a = 5.431;
  const double silicon_b = 5.431;
  const double silicon_c = 5.431;
  const double silicon_alpha = 90;
  const double silicon_beta = 90;
  const double silicon_gamma = 90;

  // constants that select the recriprocal space
  const double dspacing_min = 1.0;
  const double dspacing_max = 10.0;
  const double wavelength_min = 0.8;
  const double wavelength_max = 2.9;

  void generateSimulatedworkspace(const std::string &WSName){
    // create simulated workspace
    IAlgorithm_sptr csws_alg = 
      AlgorithmFactory::Instance().create("CreateSimulationWorkspace", 1);
    csws_alg->initialize();
    csws_alg->setProperty("Instrument", "CORELLI");
    csws_alg->setProperty("BinParams", "1,100,10000");
    csws_alg->setProperty("UnitX", "TOF");
    csws_alg->setProperty("OutputWorkspace", WSName);
    csws_alg->execute();

    // set UB
    IAlgorithm_sptr sub_alg = 
      AlgorithmFactory::Instance().create("SetUB", 1);
    sub_alg->initialize();
    sub_alg->setProperty("Workspace", WSName);
    sub_alg->setProperty("u", "1,0,0");
    sub_alg->setProperty("v", "0,1,0");
    sub_alg->setProperty("a", silicon_a);
    sub_alg->setProperty("b", silicon_b);
    sub_alg->setProperty("c", silicon_c);
    sub_alg->setProperty("alpha", silicon_alpha);
    sub_alg->setProperty("beta", silicon_beta);
    sub_alg->setProperty("gamma", silicon_gamma);
    sub_alg->execute();

    //
  }

};
