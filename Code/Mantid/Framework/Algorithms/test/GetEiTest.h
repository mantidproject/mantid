#ifndef GETE_ITEST_H_
#define GETE_ITEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/GetEi2.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/UnitFactory.h"
#include <cmath>

#include "MantidNexus/SaveNeXus.h"


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
    Workspace2D_sptr testWS = createTestWorkspaceWithMonitors();
    // This algorithm needs a name attached to the workspace
    const std::string outputName("eitest");
    AnalysisDataService::Instance().add(outputName, testWS);

    GetEi2 alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", outputName);
    alg.setProperty("Monitor1Spec", 0);
    alg.setProperty("Monitor2Spec", 1);
    alg.setProperty("EnergyEstimate", 15.0);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Test output answers
    const double expected_ei = 15.00322845;
    const double ei = alg.getProperty("IncidentEnergy");
    
    TS_ASSERT_DELTA(ei, expected_ei, 1e-08);
    // and verify it has been store on the run object
    Property *ei_runprop = testWS->run().getProperty("Ei");
    PropertyWithValue<double> *ei_propvalue = dynamic_cast<PropertyWithValue<double> *>(ei_runprop);
    TS_ASSERT_DELTA((*ei_propvalue)(), expected_ei, 1e-08);

    // T0 value
    const double tzero = alg.getProperty("Tzero");
    const double expected_tzero = 3.2641273;
    TS_ASSERT_DELTA(tzero, expected_tzero, 1e-08);

    AnalysisDataService::Instance().remove(outputName);
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
