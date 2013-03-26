#ifndef GETE_ITEST_H_
#define GETE_ITEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/GetEi2.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/UnitFactory.h"
#include <cmath>
#include "MantidDataHandling/LoadNexusMonitors.h"
#include "MantidDataHandling/SaveNexus.h"


using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::MantidVecPtr;

class GetEiTest : public CxxTest::TestSuite
{
public:

  void test_Result_For_Good_Estimate()
  {
    const double input_ei=15.0;
    const bool fixei=false;
    do_test_on_result_values(input_ei, fixei);

  }

  void test_Result_When_Fixing_Ei()
  {
    const double input_ei=15.0;
    const bool fixei=true;
    do_test_on_result_values(input_ei, fixei);
  }

  void do_test_on_result_values(double input_ei, bool fixei)
  {
    Workspace2D_sptr testWS = createTestWorkspaceWithMonitors();
    // This algorithm needs a name attached to the workspace
    const std::string outputName("eitest");
    AnalysisDataService::Instance().add(outputName, testWS);

    GetEi2 alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", outputName);
    alg.setProperty("Monitor1Spec", 1);
    alg.setProperty("Monitor2Spec", 2);
    alg.setProperty("EnergyEstimate", input_ei);
    alg.setProperty("FixEi", fixei);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Test output answers
    // The monitor peak should always be calculated from the data
    const double expected_mon_peak = 6496.00571578;
    const int expected_mon_index = 0;
    const double expected_ei = (fixei) ? input_ei : 15.00322845;
    const double ei = alg.getProperty("IncidentEnergy");
    const double first_mon_peak = alg.getProperty("FirstMonitorPeak");
    const int mon_index = alg.getProperty("FirstMonitorIndex");

    TS_ASSERT_DELTA(ei, expected_ei, 1e-08);
    TS_ASSERT_DELTA(first_mon_peak, expected_mon_peak, 1e-08);
    TS_ASSERT_EQUALS(mon_index, expected_mon_index);
    // and verify it has been store on the run object
    Property *ei_runprop = testWS->run().getProperty("Ei");
    PropertyWithValue<double> *ei_propvalue = dynamic_cast<PropertyWithValue<double> *>(ei_runprop);
    TS_ASSERT_DELTA((*ei_propvalue)(), expected_ei, 1e-08);

    const Mantid::Kernel::Property *tzeroProp = alg.getProperty("Tzero");
    if(fixei)
    {
      TS_ASSERT(tzeroProp->isDefault());
    }
    else
    {
      // T0 value
      const double tzero = alg.getProperty("Tzero");
      const double expected_tzero = 3.2641273;
      TS_ASSERT_DELTA(tzero, expected_tzero, 1e-08);
    }

    AnalysisDataService::Instance().remove(outputName);
  }


  void testParametersOnWorkspace()
  {
    Mantid::DataObjects::Workspace2D_sptr testWS = createTestWorkspaceWithMonitors();

    testWS->instrumentParameters().addString(testWS->getInstrument()->getChild(0).get(),"ei-mon1-spec", "1");
    testWS->instrumentParameters().addString(testWS->getInstrument()->getChild(0).get(),"ei-mon2-spec", "2");
    Property *incident_energy_guess = new PropertyWithValue<double>("EnergyRequest",15.0,Direction::Input);
    testWS->mutableRun().addProperty(incident_energy_guess, true);



    // This algorithm needs a name attached to the workspace
    const std::string outputName("eiNoParTest");
    AnalysisDataService::Instance().add(outputName, testWS);

    GetEi2 alg1;
    alg1.initialize();
    alg1.setPropertyValue("InputWorkspace", outputName);
    //alg1.setProperty("Monitor1Spec", 1);
    alg1.setProperty("Monitor2Spec", 2);
    alg1.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg1.execute());

    // Test output answers
    const double expected_ei = 15.00322845;
    const double ei = alg1.getProperty("IncidentEnergy");

    TS_ASSERT_DELTA(ei, expected_ei, 1e-08);
    // and verify it has been store on the run object
    Property *ei_runprop = testWS->run().getProperty("Ei");
    PropertyWithValue<double> *ei_propvalue = dynamic_cast<PropertyWithValue<double> *>(ei_runprop);
    TS_ASSERT_DELTA((*ei_propvalue)(), expected_ei, 1e-08);

    // T0 value
    const double tzero = alg1.getProperty("Tzero");
    const double expected_tzero = 3.2641273;
    TS_ASSERT_DELTA(tzero, expected_tzero, 1e-08);

    AnalysisDataService::Instance().remove(outputName);
  }

  void testThrowsMon1()
  {
      Workspace2D_sptr testWS = createTestWorkspaceWithMonitors();
      // This algorithm needs a name attached to the workspace
      const std::string outputName("eitest1");
      AnalysisDataService::Instance().add(outputName, testWS);

      GetEi2 alg;
      alg.initialize();
      alg.setPropertyValue("InputWorkspace", outputName);
      alg.setProperty("Monitor2Spec", 2);
      alg.setProperty("EnergyEstimate", 15.0);
      alg.setRethrows(true);
      TS_ASSERT_THROWS_EQUALS(alg.execute(), const std::invalid_argument &e, std::string(e.what()), "Could not determine spectrum number to use. Try to set it explicitly" );
      AnalysisDataService::Instance().remove(outputName);
  }
  void testThrowsEi()
  {
      Workspace2D_sptr testWS = createTestWorkspaceWithMonitors();
      // This algorithm needs a name attached to the workspace
      const std::string outputName("eitest2");
      AnalysisDataService::Instance().add(outputName, testWS);

      GetEi2 alg;
      alg.initialize();
      alg.setPropertyValue("InputWorkspace", outputName);
      alg.setProperty("Monitor1Spec", 1);
      alg.setProperty("Monitor2Spec", 2);
      alg.setRethrows(true);
      TS_ASSERT_THROWS_EQUALS(alg.execute(), const std::invalid_argument &e, std::string(e.what()), "Could not find an energy guess" );
      AnalysisDataService::Instance().remove(outputName);
  }

  void testCNCS()
  {
      Mantid::DataHandling::LoadNexusMonitors ld;
      std::string outws_name = "cncs";
      ld.initialize();
      ld.setPropertyValue("Filename","CNCS_7860_event.nxs");
      ld.setPropertyValue("OutputWorkspace", outws_name);

      ld.execute();
      TS_ASSERT( ld.isExecuted() );

      GetEi2 alg;
      alg.initialize();
      alg.setPropertyValue("InputWorkspace", outws_name);
      TS_ASSERT_THROWS_NOTHING(alg.execute());


      // T0 value
      const double tzero = alg.getProperty("Tzero");
      const double expected_tzero = 61.7708;
      TS_ASSERT_DELTA(tzero, expected_tzero, 1e-04);

      AnalysisDataService::Instance().remove(outws_name);
  }

private:

  Workspace2D_sptr createTestWorkspaceWithMonitors()
  {
    const int numHists(2);
    const int numBins(2000);
    Workspace2D_sptr testWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(numHists, numBins, true);
    testWS->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("TOF");
    MantidVecPtr xdata;
    xdata.access().resize(numBins+1);
    // Update X data  to a sensible values. Looks roughly like the MARI binning
    // Update the Y values. We don't care about errors here

    // Instrument geometry + incident energy of ~15 mev (purely made up) gives these neceesary peak values.
    // We'll simply use a gaussian as a test

    const double peakOneCentre(6493.0), sigmaSqOne(250*250.), peakTwoCentre(10625.), sigmaSqTwo(50*50);
    const double peakOneHeight(3000.), peakTwoHeight(1000.);
    for( int i = 0; i <= numBins; ++i)
    {
      const double xValue = 5.0 + 5.5*i;
      if( i < numBins )
      {
        testWS->dataY(0)[i] = peakOneHeight * exp(-0.5*pow(xValue - peakOneCentre, 2.)/sigmaSqOne);
        testWS->dataY(1)[i] = peakTwoHeight * exp(-0.5*pow(xValue - peakTwoCentre, 2.)/sigmaSqTwo);

      }
      xdata.access()[i] = xValue;
    }    
    testWS->setX(0, xdata);
    testWS->setX(1, xdata);
    return testWS;
  }

};

#endif /*GETE_ITEST_H_*/
