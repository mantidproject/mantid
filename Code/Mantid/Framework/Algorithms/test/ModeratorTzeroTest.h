#ifndef MANTID_ALGORITHMS_MODERATORTZEROTEST_H_
#define MANTID_ALGORITHMS_MODERATORTZEROTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/Events.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAlgorithms/ModeratorTzero.h"
#include <cmath>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Algorithms;

class ModeratorTzeroTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
static ModeratorTzeroTest *createSuite() { return new ModeratorTzeroTest(); }
static void destroySuite( ModeratorTzeroTest *suite ) { delete suite; }


ModeratorTzeroTest()
{
  FrameworkManager::Instance(); // Load plugins
}

void TestInit()
{
  ModeratorTzero alg;
  TS_ASSERT_THROWS_NOTHING(alg.initialize());
  TS_ASSERT( alg.isInitialized() );
}

void TestExecThrowsDeltaEmode()
{
  MatrixWorkspace_sptr testWS = CreateHistogramWorkspace();
  AnalysisDataService::Instance().add("testWS", testWS);
  ModeratorTzero alg;
  alg.initialize();
  alg.setProperty("InputWorkspace",testWS);
  alg.setProperty("OutputWorkspace","testWS");
  alg.setRethrows(true); // necessary, otherwise the algorithm will catch all exceptions and not return them
  TS_ASSERT_THROWS(alg.execute(), Exception::InstrumentDefinitionError);
  AnalysisDataService::Instance().remove("testWS");
}

void TestExecThrowsNoFormula()
{
  MatrixWorkspace_sptr testWS = CreateHistogramWorkspace();
  AnalysisDataService::Instance().add("testWS", testWS);
  const bool add_deltaE_mode=true;
  AddToInstrument(testWS,add_deltaE_mode);
  ModeratorTzero alg;
  alg.initialize();
  alg.setProperty("InputWorkspace",testWS);
  alg.setProperty("OutputWorkspace","testWS");
  alg.setRethrows(true); // necessary, otherwise the algorithm will catch all exceptions and not return them
  TS_ASSERT_THROWS(alg.execute(), Exception::InstrumentDefinitionError);
  AnalysisDataService::Instance().remove("testWS");
}

/*
 * First spectrum is a detector. Remaining two spectra are monitors
 */
void TestExecHistogram()
{
  MatrixWorkspace_sptr testWS = CreateHistogramWorkspace();
  AnalysisDataService::Instance().add("testWS", testWS);
  const bool add_deltaE_mode=true;
  const bool add_t0_formula=true;
  AddToInstrument(testWS, add_deltaE_mode, add_t0_formula);
  ModeratorTzero alg;
  alg.initialize();
  alg.setProperty("InputWorkspace",testWS);
  alg.setProperty("OutputWorkspace","testWS");
  alg.setRethrows(true);
  TS_ASSERT_THROWS_NOTHING(alg.execute());

  // Check a few values
  double tofs[3][11]={
      {-0.218694, 1599.78, 3199.78, 4799.78, 6399.78, 7999.78, 9550.71, 11150.2, 12750.1, 14350, 15950},
      {-34.9412, 1550.24, 3150.06, 4750.03, 6350.01, 7950.01, 9550.01, 11150, 12750, 14350, 15950},
      {-9.67714, 1550.63, 3150.16, 4750.07, 6350.04, 7950.03, 9550.02, 11150, 12750, 14350, 15950}
  };
  for (size_t ihist=0; ihist<testWS->getNumberHistograms(); ++ihist)
  {
    MantidVec xarray=testWS->dataX(ihist);
    for(size_t ibin=0; ibin < xarray.size(); ibin+=400)
      TS_ASSERT_DELTA(tofs[ihist][ibin/400],xarray[ibin],0.1);
  }
  AnalysisDataService::Instance().remove("testWS");
}

void TestExecEvents()
{
  EventWorkspace_sptr testWS=CreateEventWorkspace();
  AnalysisDataService::Instance().add("testWS", testWS);
  const bool add_deltaE_mode=true;
  const bool add_t0_formula=true;
  MatrixWorkspace_sptr mtestWS=boost::dynamic_pointer_cast<MatrixWorkspace>(testWS);
  AddToInstrument(mtestWS, add_deltaE_mode, add_t0_formula);
  ModeratorTzero alg;
  alg.initialize();
  alg.setProperty("InputWorkspace",testWS);
  alg.setProperty("OutputWorkspace","testWS");
  alg.setRethrows(true);
  TS_ASSERT_THROWS_NOTHING(alg.execute());

  // Check a few values
  double tofs_a[11]={-37.5547, 1562.45, 3162.45, 4762.45, 6362.45, 7962.45, 9550.18, 11150, 12750, 14350, 15950};
  for (size_t ihist=0; ihist<testWS->getNumberHistograms(); ++ihist)
  {
    EventList &evlist=testWS->getEventList(ihist);
    MantidVec tofs_b=evlist.getTofs();
    MantidVec xarray=evlist.readX();
    for(size_t ibin=0; ibin < xarray.size(); ibin+=400)
    {
      TS_ASSERT_DELTA(tofs_a[ibin/400],xarray[ibin],0.1);
      TS_ASSERT_DELTA(tofs_a[ibin/400],tofs_b[ibin],0.2);
    }
  }
  AnalysisDataService::Instance().remove("testWS");
}

private:

MatrixWorkspace_sptr CreateHistogramWorkspace()
{
  const int numHists(3);
  const int numBins(4000);
  MatrixWorkspace_sptr testWS=WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(numHists, numBins, true);
  testWS->getAxis(0)->unit()=Mantid::Kernel::UnitFactory::Instance().create("TOF");
  MantidVecPtr xdata;
  xdata.access().resize(numBins+1);
  const double peakHeight(1000), peakCentre(7000.), sigmaSq(1000*1000.);
  for(int ibin=0; ibin<numBins; ++ibin)
  {
    const double xValue=4*ibin;
    testWS->dataY(0)[ibin]=peakHeight*exp(-0.5*pow(xValue-peakCentre, 2.)/sigmaSq);
    xdata.access()[ibin] = xValue;
  }
  xdata.access()[numBins] = 4*numBins;
  for( int ihist=0; ihist<numHists; ihist++)
    testWS->setX(ihist, xdata);
  return testWS;
}

EventWorkspace_sptr CreateEventWorkspace()
{
  const int numBanks(1), numPixels(1), numBins(4000);
  const bool clearEvents(true);
  EventWorkspace_sptr testWS=WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(numBanks,numPixels,clearEvents);
  testWS->getAxis(0)->unit()=Mantid::Kernel::UnitFactory::Instance().create("TOF");
  const size_t numHists=testWS->getNumberHistograms();
  for (size_t ihist=0; ihist<numHists; ++ihist)
  {
    EventList &evlist=testWS->getEventList(ihist);
    MantidVecPtr xdata;
    xdata.access().resize(numBins+1);
    for(int ibin=0; ibin<=numBins; ++ibin)
    {
      double tof=4*ibin;
      TofEvent tofevent(tof);
      xdata.access()[ibin]=tof;
      evlist.addEventQuickly(tofevent); // insert event
    }
    evlist.setX(xdata); // set the bins for the associated histogram
  }
  return testWS;
}

void AddToInstrument(MatrixWorkspace_sptr &testWS, const bool &add_deltaE_mode=false, const bool &add_t0_formula=false)
{
  const double evalue(2.082); // energy corresponding to the first order Bragg peak in the analyzers
  if(add_deltaE_mode)
    testWS->instrumentParameters().addString(testWS->getInstrument()->getComponentID(),"deltaE-mode", "indirect");
    for(size_t ihist=0; ihist<testWS->getNumberHistograms(); ++ihist)
      testWS->instrumentParameters().addDouble(testWS->getDetector(ihist)->getComponentID(),"Efixed",evalue);
  if(add_t0_formula)
    testWS->instrumentParameters().addString(testWS->getInstrument()->getComponentID(),"t0_formula","50.-(50./52500)*incidentEnergy");
}

};

#endif /*MANTID_ALGORITHMS_MODERATORTZEROTEST_H_*/
