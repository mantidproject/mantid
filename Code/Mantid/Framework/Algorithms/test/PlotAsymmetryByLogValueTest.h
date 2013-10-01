#ifndef PLOTASYMMETRYBYLOGVALUTEST_H_
#define PLOTASYMMETRYBYLOGVALUTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/PlotAsymmetryByLogValue.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TextAxis.h"
#include "MantidDataHandling/LoadMuonNexus.h"
#include "MantidDataHandling/LoadInstrument.h"
#include <iostream>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class PlotAsymmetryByLogValueTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PlotAsymmetryByLogValueTest *createSuite() { return new PlotAsymmetryByLogValueTest(); }
  static void destroySuite( PlotAsymmetryByLogValueTest *suite ) { delete suite; }

  PlotAsymmetryByLogValueTest()
    :firstRun("MUSR00015189.nxs"),lastRun("MUSR00015193.nxs")
  {
  }

  void testExec()
  {
    PlotAsymmetryByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("FirstRun",firstRun);
    alg.setPropertyValue("LastRun",lastRun);
    alg.setPropertyValue("OutputWorkspace","PlotAsymmetryByLogValueTest_WS");
    alg.setPropertyValue("LogValue","Field_Danfysik");
    alg.setPropertyValue("Red","2");
  alg.setPropertyValue("Green","1");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve("PlotAsymmetryByLogValueTest_WS")
      );

    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->blocksize(),5);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(),4);
    const Mantid::MantidVec& Y = outWS->readY(0);
    TS_ASSERT_DELTA(Y[0],0.0128845,0.001);
    TS_ASSERT_DELTA(Y[1],0.0224898,0.00001);
    TS_ASSERT_DELTA(Y[2],0.0387179,0.00001);
    TS_ASSERT_DELTA(Y[3],0.0545464,0.00001);
    TS_ASSERT_DELTA(Y[4],0.0906989,0.00001);

    const TextAxis* axis = dynamic_cast<const TextAxis*>(outWS->getAxis(1));
    TS_ASSERT(axis);
    if (axis)
    {
      TS_ASSERT_EQUALS(axis->length(),4);
      TS_ASSERT_EQUALS(axis->label(0),"Red-Green");
      TS_ASSERT_EQUALS(axis->label(1),"Red");
      TS_ASSERT_EQUALS(axis->label(2),"Green");
      TS_ASSERT_EQUALS(axis->label(3),"Red+Green");
    }
    AnalysisDataService::Instance().clear();
  }
  
  void testDifferential()
  {
    PlotAsymmetryByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("FirstRun",firstRun);
    alg.setPropertyValue("LastRun",lastRun);
    alg.setPropertyValue("OutputWorkspace","PlotAsymmetryByLogValueTest_WS");
    alg.setPropertyValue("LogValue","Field_Danfysik");
    alg.setPropertyValue("Red","2");
    alg.setPropertyValue("Green","1");
    alg.setPropertyValue("Type","Differential");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve("PlotAsymmetryByLogValueTest_WS")
      );

    TS_ASSERT(outWS);
    TS_ASSERT_EQUALS(outWS->blocksize(),5);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(),4);
    const Mantid::MantidVec& Y = outWS->readY(0);
    TS_ASSERT_DELTA(Y[0],-0.01236,0.001);
    TS_ASSERT_DELTA(Y[1],0.019186,0.00001);
    TS_ASSERT_DELTA(Y[2],0.020093,0.00001);
    TS_ASSERT_DELTA(Y[3],0.037658,0.00001);
    TS_ASSERT_DELTA(Y[4],0.085060,0.00001);

    AnalysisDataService::Instance().clear();
  }

  void test_int_log()
  {
    PlotAsymmetryByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("FirstRun",firstRun);
    alg.setPropertyValue("LastRun",lastRun);
    alg.setPropertyValue("OutputWorkspace","PlotAsymmetryByLogValueTest_WS");
    alg.setPropertyValue("LogValue","nspectra");
    alg.setPropertyValue("Red","2");
    alg.setPropertyValue("Green","1");
    alg.execute();

    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve("PlotAsymmetryByLogValueTest_WS")
      );

    TS_ASSERT(outWS);
    AnalysisDataService::Instance().clear();
  }

  void test_string_log()
  {
    PlotAsymmetryByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("FirstRun",firstRun);
    alg.setPropertyValue("LastRun",lastRun);
    alg.setPropertyValue("OutputWorkspace","PlotAsymmetryByLogValueTest_WS");
    alg.setPropertyValue("LogValue","run_number");
    alg.setPropertyValue("Red","2");
    alg.setPropertyValue("Green","1");
    alg.execute();

    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve("PlotAsymmetryByLogValueTest_WS")
      );

    TS_ASSERT(outWS);
    AnalysisDataService::Instance().clear();
  }

  void test_text_log()
  {
    PlotAsymmetryByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("FirstRun",firstRun);
    alg.setPropertyValue("LastRun",lastRun);
    alg.setPropertyValue("OutputWorkspace","PlotAsymmetryByLogValueTest_WS");
    alg.setPropertyValue("LogValue","run_title");
    alg.setPropertyValue("Red","2");
    alg.setPropertyValue("Green","1");
    alg.execute();

    TS_ASSERT( ! alg.isExecuted() );

    AnalysisDataService::Instance().clear();
  }

private:
  std::string firstRun,lastRun;
  
};

#endif /*PLOTASYMMETRYBYLOGVALUTEST_H_*/
