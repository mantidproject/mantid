#ifndef PLOTASYMMETRYBYLOGVALUTEST_H_
#define PLOTASYMMETRYBYLOGVALUTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/PlotAsymmetryByLogValue.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataHandling/LoadMuonNexus.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/SaveNexus.h"

#include <iostream>
#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

class PlotAsymmetryByLogValueTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PlotAsymmetryByLogValueTest *createSuite() { return new PlotAsymmetryByLogValueTest(); }
  static void destroySuite( PlotAsymmetryByLogValueTest *suite ) { delete suite; }

  PlotAsymmetryByLogValueTest()
    :firstRun("MUSR00015189.nxs"),lastRun("MUSR00015190.nxs")
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
    TS_ASSERT_EQUALS(outWS->blocksize(),2);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(),4);
    const Mantid::MantidVec& Y = outWS->readY(0);
    TS_ASSERT_DELTA(Y[0],0.0128845,0.001);
    TS_ASSERT_DELTA(Y[1],0.0224898,0.00001);

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
    TS_ASSERT_EQUALS(outWS->blocksize(),2);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(),4);
    const Mantid::MantidVec& Y = outWS->readY(0);
    TS_ASSERT_DELTA(Y[0],-0.01236,0.001);
    TS_ASSERT_DELTA(Y[1],0.019186,0.00001);

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

  void test_DeadTimeCorrection_FromSpecifiedFile()
  {
    const std::string ws = "Ws";
    const std::string deadTimeWs = "DeadTimeWs";
    const std::string deadTimeFile = "TestDeadTimeFile.nxs";

    ITableWorkspace_sptr deadTimeTable = Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
    deadTimeTable->addColumn("int","spectrum");
    deadTimeTable->addColumn("double","dead-time");

    for(int i = 0; i < 64; i++)
    {
      TableRow row = deadTimeTable->appendRow();
      row << (i+1) << 0.015;
    }

    AnalysisDataService::Instance().addOrReplace(deadTimeWs, deadTimeTable);

    // Save dead time table to file
    SaveNexus saveNexusAlg;
    TS_ASSERT_THROWS_NOTHING(saveNexusAlg.initialize());
    saveNexusAlg.setPropertyValue("InputWorkspace", deadTimeWs);
    saveNexusAlg.setPropertyValue("Filename", deadTimeFile);
    TS_ASSERT_THROWS_NOTHING(saveNexusAlg.execute());
    TS_ASSERT(saveNexusAlg.isExecuted());

    PlotAsymmetryByLogValue alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setPropertyValue("FirstRun", firstRun);
    alg.setPropertyValue("LastRun", lastRun);
    alg.setPropertyValue("OutputWorkspace", ws);
    alg.setPropertyValue("LogValue", "run_number");
    alg.setPropertyValue("DeadTimeCorrType", "FromSpecifiedFile");
    alg.setPropertyValue("DeadTimeCorrFile", deadTimeFile);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    if(!alg.isExecuted()) return;

    MatrixWorkspace_sptr outWs = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve(ws));

    TS_ASSERT(outWs);
    TS_ASSERT_EQUALS(outWs->blocksize(), 2);
    TS_ASSERT_EQUALS(outWs->getNumberHistograms(),1);

    const Mantid::MantidVec& Y = outWs->readY(0);

    TS_ASSERT_DELTA(Y[0], 0.15108, 0.00001);
    TS_ASSERT_DELTA(Y[1], 0.14389, 0.00001);

    AnalysisDataService::Instance().remove(ws);
    AnalysisDataService::Instance().remove(deadTimeWs);
    Poco::File(deadTimeFile).remove();
  }

  void test_DeadTimeCorrection_FromRunData()
  {
    const std::string ws = "Test_DeadTimeCorrection_FromRunData_Ws";

    PlotAsymmetryByLogValue alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setPropertyValue("FirstRun", firstRun);
    alg.setPropertyValue("LastRun", lastRun);
    alg.setPropertyValue("OutputWorkspace", ws);
    alg.setPropertyValue("LogValue","run_number");
    alg.setPropertyValue("DeadTimeCorrType","FromRunData");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    if(!alg.isExecuted()) return;

    MatrixWorkspace_sptr outWs = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve(ws));

    TS_ASSERT(outWs);
    TS_ASSERT_EQUALS(outWs->blocksize(), 2);
    TS_ASSERT_EQUALS(outWs->getNumberHistograms(),1);

    const Mantid::MantidVec& Y = outWs->readY(0);

    TS_ASSERT_DELTA(Y[0], 0.150616, 0.00001);
    TS_ASSERT_DELTA(Y[1], 0.143444, 0.00001);

    AnalysisDataService::Instance().remove(ws);
  }

  void test_LogValueFunction ()
  {
    const std::string ws = "Test_DeadTimeCorrection_FromRunData_Ws";

    PlotAsymmetryByLogValue alg;

    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    alg.setPropertyValue("FirstRun", firstRun);
    alg.setPropertyValue("LastRun", lastRun);
    alg.setPropertyValue("OutputWorkspace", ws);
    // We use 'beamlog_current' as log value because 
    // we want to test the 'Mean' function below and this is 
    // one of the few properties that contains different values over time
    alg.setPropertyValue("LogValue","beamlog_current");
    alg.setPropertyValue("Function","Mean");
    alg.setPropertyValue("DeadTimeCorrType","None");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_sptr outWs = boost::dynamic_pointer_cast<MatrixWorkspace>(
      AnalysisDataService::Instance().retrieve(ws));

    TS_ASSERT(outWs);
    TS_ASSERT_EQUALS(outWs->blocksize(), 2);
    TS_ASSERT_EQUALS(outWs->getNumberHistograms(),1);

    // Now we want to test X values (log values) in the output workspace
    // rather than asymmetry (Y values)
    const Mantid::MantidVec& X = outWs->readX(0);

    TS_ASSERT_DELTA(X[0], 179.078620, 0.00001);
    TS_ASSERT_DELTA(X[1], 178.849998, 0.00001);

    AnalysisDataService::Instance().remove(ws);
  }

private:
  std::string firstRun,lastRun;
  
};

#endif /*PLOTASYMMETRYBYLOGVALUTEST_H_*/
