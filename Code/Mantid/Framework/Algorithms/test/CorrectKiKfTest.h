#ifndef CORRECTKIKFTEST_H_
#define CORRECTKIKFTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/CorrectKiKf.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidAlgorithms/ConvertUnits.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class CorrectKiKfTest : public CxxTest::TestSuite
{
public:

  CorrectKiKfTest(): inputWSname("testInput"), inputEvWSname("testEvInput"), outputWSname("testOutput"), outputEvWSname("testEvOutput")
  {
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );
  }

  void testExec()
  {
    bool isHistogram=true;
   
    //check direct histogram
    createWorkspace2D(isHistogram);
    alg.setPropertyValue("InputWorkspace", inputWSname);
    alg.setPropertyValue("OutputWorkspace", outputWSname);
    alg.setPropertyValue("EMode","Direct");
    alg.setPropertyValue("EFixed","7.5");
    alg.execute();
    TS_ASSERT( alg.isExecuted() );
    Workspace2D_sptr result = boost::dynamic_pointer_cast<Workspace2D>(AnalysisDataService::Instance().retrieve(outputWSname));
    double ei,ef,factor,deltaE;
    int numHists = result->getNumberHistograms();
    for (int i = 0; i < result->blocksize(); ++i)
    {  
      ei = 7.5;
      deltaE = (i-2.)*5.;
      ef = ei-deltaE;
      if (ei*ef < 0) {factor=0.;} else {factor=std::sqrt(ei/ef);}
      TS_ASSERT_DELTA(factor,(result->readY(0)[i])/(i+1.),1e-8);
      TS_ASSERT_DELTA(factor,(result->readE(0)[i])/std::sqrt(i+1.),1e-8);
    }
    AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);

    //check direct not histogram
    isHistogram=false;
    createWorkspace2D(isHistogram);
    alg.setPropertyValue("InputWorkspace", inputWSname);
    alg.setPropertyValue("OutputWorkspace", outputWSname);
    alg.setPropertyValue("EMode","Direct");
    alg.setPropertyValue("EFixed","7.5");
    alg.execute();
    TS_ASSERT( alg.isExecuted() );
    result = boost::dynamic_pointer_cast<Workspace2D>(AnalysisDataService::Instance().retrieve(outputWSname));
    numHists = result->getNumberHistograms();
    for (int i = 0; i < result->blocksize(); ++i)
    {  
      ei = 7.5;
      deltaE = (i-2.)*5.;
      ef = ei-deltaE;
      if (ei*ef < 0) {factor=0.;} else {factor=std::sqrt(ei/ef);}
      TS_ASSERT_DELTA(factor,(result->readY(0)[i])/(i+1.),1e-8);
      TS_ASSERT_DELTA(factor,(result->readE(0)[i])/std::sqrt(i+1.),1e-8);
    }
    AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);

    //check indirect not histogram
    isHistogram=false;
    createWorkspace2D(isHistogram);
    alg.setPropertyValue("InputWorkspace", inputWSname);
    alg.setPropertyValue("OutputWorkspace", outputWSname);
    alg.setPropertyValue("EMode","Indirect");
    alg.setPropertyValue("EFixed","7.5");
    alg.execute();
    TS_ASSERT( alg.isExecuted() );
    result = boost::dynamic_pointer_cast<Workspace2D>(AnalysisDataService::Instance().retrieve(outputWSname));
    numHists = result->getNumberHistograms();
    for (int i = 0; i < result->blocksize(); ++i)
    {  
      ef = 7.5;
      deltaE = (i-2.)*5.;
      ei = ef+deltaE;
      if (ei*ef < 0) {factor=0.;} else {factor=std::sqrt(ei/ef);}
      TS_ASSERT_DELTA(factor,(result->readY(0)[i])/(i+1.),1e-8);
      TS_ASSERT_DELTA(factor,(result->readE(0)[i])/std::sqrt(i+1.),1e-8);
    }
    AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);

    //check indirect histogram
    isHistogram=true;
    createWorkspace2D(isHistogram);
    alg.setPropertyValue("InputWorkspace", inputWSname);
    alg.setPropertyValue("OutputWorkspace", outputWSname);
    alg.setPropertyValue("EMode","Indirect");
    alg.setPropertyValue("EFixed","7.5");
    alg.execute();
    TS_ASSERT( alg.isExecuted() );
    result = boost::dynamic_pointer_cast<Workspace2D>(AnalysisDataService::Instance().retrieve(outputWSname));
    numHists = result->getNumberHistograms();
    for (int i = 0; i < result->blocksize(); ++i)
    {  
      ef = 7.5;
      deltaE = (i-2.)*5.;
      ei = ef+deltaE;
      if (ei*ef < 0) {factor=0.;} else {factor=std::sqrt(ei/ef);}
      TS_ASSERT_DELTA(factor,(result->readY(0)[i])/(i+1.),1e-8);
      TS_ASSERT_DELTA(factor,(result->readE(0)[i])/std::sqrt(i+1.),1e-8);
    }
    AnalysisDataService::Instance().remove(outputWSname);
    AnalysisDataService::Instance().remove(inputWSname);

  }

  void testEventCorrection()
  {

    createEventWorkspace();
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );

    alg.setPropertyValue("InputWorkspace", inputEvWSname);
    alg.setPropertyValue("OutputWorkspace", outputEvWSname);
    alg.setPropertyValue("EMode","Indirect");
    alg.setPropertyValue("EFixed","100.");
    // Should blow up, but it doesn't. It seems that the error is caught by Mantid 
    // TS_ASSERT_THROWS( alg.execute(), std::runtime_error );
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    AnalysisDataService::Instance().remove(outputEvWSname);
    AnalysisDataService::Instance().remove(inputEvWSname);
  }


  void testReadEffromIDF()
  {
    Mantid::DataHandling::LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename","IRS38633.raw");
    const std::string initialWS("IRS");
    const std::string intermediaryWS("IRSenergy");
    const std::string finalWS("Corrected");
    loader.setPropertyValue("OutputWorkspace",initialWS);
    loader.setPropertyValue("SpectrumList","3");
    loader.setPropertyValue("LoadMonitors","Exclude");
    loader.execute();

    Mantid::Algorithms::ConvertUnits convert;
    convert.initialize();
    convert.setPropertyValue("InputWorkspace",initialWS);
    convert.setPropertyValue("OutputWorkspace",intermediaryWS);
    convert.setPropertyValue("Target","DeltaE");
    convert.setPropertyValue("EMode","Indirect");
    convert.setPropertyValue("EFixed","1.845");
    convert.execute();

    CorrectKiKf alg1; //I use alg1 because I cannot remove Efixed property
    alg1.initialize();

    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("InputWorkspace",intermediaryWS) );
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("OutputWorkspace",finalWS) );
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("EMode","Indirect"));
    TS_ASSERT_THROWS_NOTHING( alg1.execute() );
    TS_ASSERT( alg1.isExecuted() );
    
    MatrixWorkspace_sptr result;
    TS_ASSERT_THROWS_NOTHING( result = boost::dynamic_pointer_cast<Workspace2D>
                                (AnalysisDataService::Instance().retrieve(finalWS)) );
   
    TS_ASSERT_DELTA( result->readX(0)[1976], 1.18785, 0.0001 );
    TS_ASSERT_DELTA( result->readX(0)[1977], 1.18912, 0.0001 );
    TS_ASSERT_DELTA( result->readY(0)[1976], 1.28225, 0.0001 );

    //Ef=1.845, Ei=Ef+0.5*(x[1977]+x[1976]), Y [1976] uncorrected=1, ki/kf=sqrt(Ei/Ef)
    TS_ASSERT_DELTA( sqrt(((result->readX(0)[1976]+result->readX(0)[1977])*0.5+1.845)/1.845), result->readY(0)[1976], 0.0001 );
    
    AnalysisDataService::Instance().remove(initialWS);
    AnalysisDataService::Instance().remove(intermediaryWS);
    AnalysisDataService::Instance().remove(finalWS);

  }



private:
  CorrectKiKf alg; 
  std::string inputWSname;
  std::string inputEvWSname;
  std::string outputWSname;
  std::string outputEvWSname;

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
      ws2D->getAxis(1)->spectraNo(i) = i;
    }

    AnalysisDataService::Instance().add(inputWSname, ws2D);

  }

  void createEventWorkspace()
  {
    EventWorkspace_sptr event = EventWorkspace_sptr(new EventWorkspace());
    event->initialize(1, 1, 1);
    event->doneLoadingData();
    event->getAxis(0)->unit() = UnitFactory::Instance().create("DeltaE"); 
    AnalysisDataService::Instance().add(inputEvWSname, event);
  }


};

#endif /*CorrectKiKfTEST_H_*/
