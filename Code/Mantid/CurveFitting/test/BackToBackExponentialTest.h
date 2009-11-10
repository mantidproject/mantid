#ifndef BACKTOBACKEXPONENTIALTEST_H_
#define BACKTOBACKEXPONENTIALTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/BackToBackExponential.h"
#include "MantidCurveFitting/LinearBackground.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRaw.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;


class BackToBackExponentialTest : public CxxTest::TestSuite
{
public:

  void testAgainstHRPDdataPeak()
  {
    // load dataset
    std::string inputFile = "../../../../Test/Data/HRP38692.RAW";
    LoadRaw loader;
    loader.initialize();
    loader.setPropertyValue("Filename", inputFile);
    std::string outputSpace = "HRP38692";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    loader.execute();

    // Setup Fit algorithm
    Fit alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );

    alg.setPropertyValue("InputWorkspace",outputSpace);
    alg.setPropertyValue("WorkspaceIndex","2");
    alg.setPropertyValue("StartX","79280"); 
    alg.setPropertyValue("EndX","79615");   

    // create function you want to fit against
    CompositeFunction *fnWithBk = new CompositeFunction();

    LinearBackground *bk = new LinearBackground();
    bk->initialize();

    bk->getParameter("A0") = 8.0;
    bk->getParameter("A1") = 0.0;
    bk->removeActive(1);    

    BackToBackExponential* fn = new BackToBackExponential();
    fn->initialize();

    fn->getParameter("I") = 297.0;
    fn->getParameter("A") = 2.0;
    fn->getParameter("B") = 0.03;
    fn->getParameter("X0") = 79400.0;
    fn->getParameter("S") = 8.0;

    fnWithBk->addFunction(fn);
    fnWithBk->addFunction(bk);

    alg.setFunction(fnWithBk);

    // execute fit
    TS_ASSERT_THROWS_NOTHING( alg.execute());
    TS_ASSERT( alg.isExecuted() );

    // did you get what you expected
    double dummy = alg.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 9.15,0.1);

    TS_ASSERT_DELTA( fn->getParameter("I"), 294.37 ,0.1);
    TS_ASSERT_DELTA( fn->getParameter("A"), 2.38 ,0.1);
    TS_ASSERT_DELTA( fn->getParameter("B"), 0.03 ,0.1);
    TS_ASSERT_DELTA( fn->getParameter("X0"), 79400.02 ,0.1);
    TS_ASSERT_DELTA( fn->getParameter("S"), 8.15 ,0.1);

    TS_ASSERT_DELTA( bk->getParameter("A0"), 7.88 ,0.1);
  }

};

#endif /*BACKTOBACKEXPONENTIALTEST_H_*/
