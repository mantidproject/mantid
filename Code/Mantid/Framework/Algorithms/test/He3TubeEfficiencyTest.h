#ifndef HE3TUBEEFFICIENCYTEST_H_
#define HE3TUBEEFFICIENCYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/He3TubeEfficiency.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid::Kernel;
using namespace Mantid::Kernel::Exception;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace std;

class He3TubeEfficiencyTest : public CxxTest::TestSuite
{
public:
  He3TubeEfficiencyTest() : inputWS("testInput"), inputEvWS("testEvInput")
  {
  }

  void testCorrection()
  {
    createWorkspace2D();
    He3TubeEfficiency alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );

    alg.setPropertyValue("InputWorkspace", inputWS);
    alg.setPropertyValue("OutputWorkspace", inputWS);

    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    MatrixWorkspace_sptr result = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(inputWS));

    // Monitor should be untouched
    TS_ASSERT_DELTA(result->readY(0).front(), 10.0, 1e-6);
    // Check some detector values
    TS_ASSERT_DELTA(result->readY(1).back(), 15.989063, 1e-6);
    TS_ASSERT_DELTA(result->readY(2)[2], 21.520201, 1e-6);
    TS_ASSERT_DELTA(result->readY(3).front(), 31.716197, 1e-6);

    AnalysisDataService::Instance().remove(inputWS);
  }

  void testEventCorrection()
  {
    createEventWorkspace();
    He3TubeEfficiency alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );

    alg.setPropertyValue("InputWorkspace", inputEvWS);
    alg.setPropertyValue("OutputWorkspace", inputEvWS);

    alg.execute();
    TS_ASSERT( !alg.isExecuted() );

    AnalysisDataService::Instance().remove(inputEvWS);
  }

  void testBadOverrideParameters()
  {
    createWorkspace2D();
    He3TubeEfficiency alg;
    alg.initialize();

    TS_ASSERT_THROWS(alg.setPropertyValue("TubePressure", "-10"),
        invalid_argument);
    TS_ASSERT_THROWS(alg.setPropertyValue("TubeThickness", "-0.08"),
        invalid_argument);
    TS_ASSERT_THROWS(alg.setPropertyValue("TubeTemperature", "-100"),
        invalid_argument);

    AnalysisDataService::Instance().remove(inputWS);
  }

  void testBadTubeThickness()
  {
    createWorkspace2D();
    He3TubeEfficiency alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );

    alg.setPropertyValue("InputWorkspace", inputWS);
    alg.setPropertyValue("OutputWorkspace", inputWS);
    alg.setPropertyValue("TubeThickness", "0.0127");

    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    MatrixWorkspace_sptr result = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(inputWS));

    // Monitor should be untouched
    TS_ASSERT_DELTA(result->readY(0).front(), 10.0, 1e-6);
    // Check that detector values should be zero
    TS_ASSERT_DELTA(result->readY(1).back(), 0.0, 1e-6);
    TS_ASSERT_DELTA(result->readY(2)[2], 0.0, 1e-6);
    TS_ASSERT_DELTA(result->readY(3).front(), 0.0, 1e-6);

    AnalysisDataService::Instance().remove(inputWS);
  }

private:
  const std::string inputWS;
  const std::string inputEvWS;

  void createWorkspace2D()
  {
    const int nspecs(4);
    const int nbins(5);

    MatrixWorkspace_sptr space = WorkspaceFactory::Instance().create("Workspace2D",
        nspecs, nbins + 1, nbins);
    space->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);

    Mantid::MantidVecPtr x,y,e;
    x.access().resize(nbins + 1, 0.0);
    y.access().resize(nbins, 0.0);
    e.access().resize(nbins, 0.0);
    for (int i = 0; i < nbins; ++i)
    {
      x.access()[i] = static_cast<double>((1. + i) / 10.);
      y.access()[i] = 10.0;
      e.access()[i] = sqrt(5.0);
    }
    x.access()[nbins] = static_cast<double>((1. + nbins) / 10.);

    int *specNums = new int[nspecs];
    int *detIDs = new int[nspecs];
    for (int i = 0; i < nspecs; i++)
    {
      space2D->setX(i, x);
      space2D->setData(i, y, e);
      space2D->getAxis(1)->spectraNo(i) = i;

      specNums[i] = i;
      detIDs[i] = i;
    }
    space2D->mutableSpectraMap().populate(specNums, detIDs, nspecs);
    delete specNums;
    delete detIDs;

    AnalysisDataService::Instance().add(inputWS, space2D);

    LoadInstrument loader;
    loader.initialize();
    loader.setPropertyValue("Filename",
        "../../../Instrument/IDFs_for_UNIT_TESTING/DUM_Definition.xml");
    loader.setPropertyValue("Workspace", inputWS);
    loader.execute();
  }

  void createEventWorkspace()
  {
    EventWorkspace_sptr event = EventWorkspace_sptr(new EventWorkspace());
    event->initialize(1, 1, 1);
    event->getAxis(0)->unit() = UnitFactory::Instance().create("Wavelength");
    AnalysisDataService::Instance().add(inputEvWS, event);

    LoadInstrument loader;
    loader.initialize();
    loader.setPropertyValue("Filename",
        "../../../Instrument/IDFs_for_UNIT_TESTING/DUM_Definition.xml");
    loader.setPropertyValue("Workspace", inputEvWS);
    loader.execute();
  }
};

#endif // HE3TUBEEFFICIENCYTEST_H_
