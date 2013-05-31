#ifndef MANTID_ALGORITHMS_APPLYDETAILEDBALANCETEST_H_
#define MANTID_ALGORITHMS_APPLYDETAILEDBALANCETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>
#include "MantidAPI/AlgorithmManager.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAlgorithms/ApplyDetailedBalance.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
//using namespace Mantid::DataHandling;

class ApplyDetailedBalanceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ApplyDetailedBalanceTest *createSuite() { return new ApplyDetailedBalanceTest(); }
  static void destroySuite( ApplyDetailedBalanceTest *suite ) { delete suite; }
  ApplyDetailedBalanceTest():inputWSname("testADBInput"),outputWSname("testADBOutput")
  {
  }

  void test_Init()
  {
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }


  void test_exec()
  {
    createWorkspace2D(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace",inputWSname) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace",outputWSname) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Temperature", "300.") );
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    Workspace2D_sptr inws,outws;
    TS_ASSERT_THROWS_NOTHING( outws = AnalysisDataService::Instance().retrieveWS<Workspace2D>(outputWSname) );
    TS_ASSERT(outws);
    TS_ASSERT_THROWS_NOTHING( inws = AnalysisDataService::Instance().retrieveWS<Workspace2D>(inputWSname) );
    TS_ASSERT(inws);
    if (!outws) return;
    
    for(std::size_t i=0;i<5;++i)
    {
      TS_ASSERT_DELTA(outws->readY(0)[i],M_PI*(1-std::exp(-11.604519*(inws->readX(0)[i]+inws->readX(0)[i+1])/2./300.))*inws->readY(0)[i],1e-8);
    }
    AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);
  }

  void test_failTemp()
  {
    createWorkspace2D(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace",inputWSname) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace",outputWSname) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Temperature", "x") );
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );
    Workspace2D_sptr outws;
    TS_ASSERT_THROWS_ANYTHING( outws = AnalysisDataService::Instance().retrieveWS<Workspace2D>(outputWSname) );

    //AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);
  }

  void test_event()
  {
    EventWorkspace_sptr evin=WorkspaceCreationHelper::CreateEventWorkspace(1,5,10,0,1,3),evout;
    evin->getAxis(0)->unit() = UnitFactory::Instance().create("DeltaE");
    AnalysisDataService::Instance().add(inputWSname, evin);

    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace",inputWSname));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace",outputWSname));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Temperature","100"));


    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    TS_ASSERT_THROWS_NOTHING( evout = boost::dynamic_pointer_cast<EventWorkspace>(
                                AnalysisDataService::Instance().retrieve(outputWSname)));

    double temp=100.;
    TS_ASSERT( evout ); //should be an event workspace
    for (size_t i=0;i<5;++i)
    {
      double en=static_cast<double>(i)+0.5;
      double w=M_PI*(1-std::exp(-en*11.604519/temp));
      TS_ASSERT_DELTA(evout->getEventList(0).getEvent(i).m_weight,w,w*1e-6);
    }
    AnalysisDataService::Instance().remove(outputWSname);

    AnalysisDataService::Instance().remove(inputWSname);

  }

private:
  ApplyDetailedBalance alg;
  std::string inputWSname;
  std::string outputWSname;

  void createWorkspace2D(bool isHistogram)
  {
    const int nspecs(1);
    const int nbins(5);
    double h=0;

    if(isHistogram) h=0.5;

    Workspace2D_sptr ws2D(new Workspace2D);
    ws2D->initialize(nspecs,nbins+1,nbins);
    ws2D->getAxis(0)->unit() = UnitFactory::Instance().create("DeltaE");

    Mantid::MantidVecPtr xv,yv,ev;
    if (isHistogram)
      {xv.access().resize(nbins + 1, 0.0);}
    else
      {xv.access().resize(nbins , 0.0);}
    yv.access().resize(nbins, 0.0);
    ev.access().resize(nbins, 0.0);
    for (int i = 0; i < nbins; ++i)
    {
      xv.access()[i] = static_cast<double>((i-2.-h)*5.);
      yv.access()[i] = 1.0+i;
      ev.access()[i] = std::sqrt(1.0+i);
    }
    if (isHistogram)
      {xv.access()[nbins] = static_cast<double>((nbins-2.5)*5.);}


    for (int i=0; i< nspecs; i++)
    {
      ws2D->setX(i,xv);
      ws2D->setData(i,yv,ev);
      ws2D->getSpectrum(i)->setSpectrumNo(i);
    }

    AnalysisDataService::Instance().add(inputWSname, ws2D);

  }

};


#endif /* MANTID_ALGORITHMS_APPLYDETAILEDBALANCETEST_H_ */

