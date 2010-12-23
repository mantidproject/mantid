#ifndef GAUSSIANTEST_H_
#define GAUSSIANTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Gaussian.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidCurveFitting/LinearBackground.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRaw.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ConfigService.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidAlgorithms/ConvertUnits.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::Algorithms;

// Algorithm to force Gaussian1D to be run by simplex algorithm
class SimplexGaussian : public Gaussian
{
public:
  virtual ~SimplexGaussian() {}

protected:
  void functionDeriv(Jacobian* out, const double* xValues, const int& nData)
  {
    throw Exception::NotImplementedError("No derivative function provided");
  }
};


class GaussianTest : public CxxTest::TestSuite
{
public:

  void getMockData(Mantid::MantidVec& y, Mantid::MantidVec& e)
  {
    y[0] =   3.56811123;
    y[1] =   3.25921675;
    y[2] =   2.69444562;
    y[3] =   3.05054488;
    y[4] =   2.86077216;
    y[5] =   2.29916480;
    y[6] =   2.57468876;
    y[7] =   3.65843827;
    y[8] =  15.31622763;
    y[9] =  56.57989073;
    y[10] = 101.20662386;
    y[11] =  76.30364797;
    y[12] =  31.54892552;
    y[13] =   8.09166673;
    y[14] =   3.20615343;
    y[15] =   2.95246554;
    y[16] =   2.75421444;
    y[17] =   3.70180447;
    y[18] =   2.77832668;
    y[19] =   2.29507565;

    for (int i = 0; i <=19; i++)
      y[i] -= 2.8765;

    e[0] =   1.72776328;
    e[1] =   1.74157482;
    e[2] =   1.73451042;
    e[3] =   1.73348562;
    e[4] =   1.74405622;
    e[5] =   1.72626701;
    e[6] =   1.75911386;
    e[7] =   2.11866496;
    e[8] =   4.07631054;
    e[9] =   7.65159052;
    e[10] =  10.09984173;
    e[11] =   8.95849024;
    e[12] =   5.42231173;
    e[13] =   2.64064858;
    e[14] =   1.81697576;
    e[15] =   1.72347732;
    e[16] =   1.73406310;
    e[17] =   1.73116711;
    e[18] =   1.71790285;
    e[19] =   1.72734254;
  }


  // test look up table 
  void t1estAgainstHRPD_DatasetLookUpTable()
  {
    // load dataset to test against
    std::string inputFile = "../../../../Test/AutoTestData/HRP38692.raw";
    LoadRaw loader;
    loader.initialize();
    loader.setPropertyValue("Filename", inputFile);
    std::string outputSpace = "HRP38692_Dataset";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    loader.execute();

    // reload instrument to test constraint defined in IDF is working
    LoadInstrument reLoadInstrument;
    reLoadInstrument.initialize();
    std::string instrumentName = "HRPD_for_UNIT_TESTING.xml";
    reLoadInstrument.setPropertyValue("Filename", "../../../../Test/Instrument/IDFs_for_UNIT_TESTING/" + instrumentName);
    reLoadInstrument.setPropertyValue("Workspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING(reLoadInstrument.execute());
    TS_ASSERT( reLoadInstrument.isExecuted() );

    Fit alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );

    // Set which spectrum to fit against and initial starting values
    alg.setPropertyValue("InputWorkspace",outputSpace);
    alg.setPropertyValue("WorkspaceIndex","68");
    alg.setPropertyValue("StartX","60134");
    alg.setPropertyValue("EndX","61805");

    // create function you want to fit against
    CompositeFunction *fnWithBk = new CompositeFunction();

    LinearBackground *bk = new LinearBackground();
    bk->initialize();

    bk->setParameter("A0",0.0);
    bk->setParameter("A1",0.0);
    bk->removeActive(1);  

    // set up Gaussian fitting function
    Gaussian* fn = new Gaussian();
    fn->initialize();

    fn->setParameter("Height",300.0);    
    fn->setParameter("PeakCentre",60990.0);
    //fn->setParameter("Sigma", 0.1);     // set using lookuptable

    // set the workspace explicitely just to make sure that Sigma has been
    // set as expected from the look up table
    Mantid::DataObjects::Workspace2D_sptr wsToPass = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(AnalysisDataService::Instance().retrieve(outputSpace));
    fn->setMatrixWorkspace(wsToPass,68,0,0);
    TS_ASSERT_DELTA( fn->getParameter("Sigma"), 109.9 ,0.1);

    fnWithBk->addFunction(bk);
    fnWithBk->addFunction(fn);

    alg.setFunction(fnWithBk);

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg.execute() )
    )
    TS_ASSERT( alg.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 1.43,0.1);

    TS_ASSERT_DELTA( fn->height(), 315.4 ,1);
    TS_ASSERT_DELTA( fn->centre(), 60980 ,10);
    TS_ASSERT_DELTA( fn->getParameter("Sigma"), 114.6 ,0.1);
    TS_ASSERT_DELTA( bk->getParameter("A0"), 7.4 ,0.1);
    TS_ASSERT_DELTA( bk->getParameter("A1"), 0.0 ,0.01); 

    AnalysisDataService::Instance().remove(outputSpace);
    InstrumentDataService::Instance().remove(instrumentName);
  }


  // test look up table 
  void t1estAgainstHRPD_DatasetLookUpTableDifferentUnit()
  {
    // load dataset to test against
    std::string inputFile = "../../../../Test/AutoTestData/HRP38692.raw";
    LoadRaw loader;
    loader.initialize();
    loader.setPropertyValue("Filename", inputFile);
    std::string outputSpace = "HRP38692_Dataset";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    loader.execute();

    ConvertUnits units;
    units.initialize();
    units.setPropertyValue("InputWorkspace", outputSpace);
    units.setPropertyValue("OutputWorkspace", outputSpace);
    units.setPropertyValue("Target", "Wavelength");
    units.setPropertyValue("EMode", "Direct");
    units.execute();
    TS_ASSERT( units.isExecuted() );

    // reload instrument to test constraint defined in IDF is working
    LoadInstrument reLoadInstrument;
    reLoadInstrument.initialize();
    std::string instrumentName = "HRPD_for_UNIT_TESTING2.xml";
    reLoadInstrument.setPropertyValue("Filename", "../../../../Test/Instrument/IDFs_for_UNIT_TESTING/" + instrumentName);
    reLoadInstrument.setPropertyValue("Workspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING(reLoadInstrument.execute());
    TS_ASSERT( reLoadInstrument.isExecuted() );

    Fit alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );

    // Set which spectrum to fit against and initial starting values
    alg.setPropertyValue("InputWorkspace",outputSpace);
    alg.setPropertyValue("WorkspaceIndex","68");
    alg.setPropertyValue("StartX","2.46");
    alg.setPropertyValue("EndX","2.52");

    // create function you want to fit against
    CompositeFunction *fnWithBk = new CompositeFunction();

    LinearBackground *bk = new LinearBackground();
    bk->initialize();

    bk->setParameter("A0",0.0);
    bk->setParameter("A1",0.0);
    bk->removeActive(1);  

    // set up Gaussian fitting function
    Gaussian* fn = new Gaussian();
    fn->initialize();

    //fn->setParameter("Height",300.0);    // set using lookuptable
    fn->setParameter("PeakCentre",2.5);
    fn->setParameter("Sigma",0.01);     

    // set the workspace explicitely just to make sure that Sigma has been
    // set as expected from the look up table
    Mantid::DataObjects::Workspace2D_sptr wsToPass = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(AnalysisDataService::Instance().retrieve(outputSpace));
    fn->setMatrixWorkspace(wsToPass,68,0,0);
    TS_ASSERT_DELTA( fn->getParameter("Height"), 317.23 ,0.1);

    fnWithBk->addFunction(bk);
    fnWithBk->addFunction(fn);

    alg.setFunction(fnWithBk);

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg.execute() )
    )
    TS_ASSERT( alg.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 1.43,0.1);

    TS_ASSERT_DELTA( fn->height(), 315.4 ,1);
    TS_ASSERT_DELTA( fn->centre(), 2.5 ,0.01);
    TS_ASSERT_DELTA( fn->getParameter("Sigma"), 0.0046 ,0.001);
    TS_ASSERT_DELTA( bk->getParameter("A0"), 7.2654 ,0.1);
    TS_ASSERT_DELTA( bk->getParameter("A1"), 0.0 ,0.01); 

    AnalysisDataService::Instance().remove(outputSpace);
    InstrumentDataService::Instance().remove(instrumentName);
  }


  // Also pick values from HRPD_for_UNIT_TESTING.xml
  // here we have an example where an upper constraint on Sigma <= 100 makes
  // the Gaussian fit below success. The starting value of Sigma is here 300. 
  // Note that the fit is equally successful if we had no constraint on Sigma
  // and used a starting of Sigma = 100.
  void t1estAgainstHRPD_DatasetWithConstraints()
  {
    // load dataset to test against
    std::string inputFile = "../../../../Test/AutoTestData/HRP38692.raw";
    LoadRaw loader;
    loader.initialize();
    loader.setPropertyValue("Filename", inputFile);
    std::string outputSpace = "HRP38692_Dataset";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    loader.execute();

    //int iii;
    //std::cin >> iii;
    // reload instrument to test constraint defined in IDF is working
    LoadInstrument reLoadInstrument;
    reLoadInstrument.initialize();
    std::string instrumentName = "HRPD_for_UNIT_TESTING.xml";
    reLoadInstrument.setPropertyValue("Filename", "../../../../Test/Instrument/IDFs_for_UNIT_TESTING/" + instrumentName);
    reLoadInstrument.setPropertyValue("Workspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING(reLoadInstrument.execute());
    TS_ASSERT( reLoadInstrument.isExecuted() );

    Fit alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );

    // Set which spectrum to fit against and initial starting values
    alg.setPropertyValue("InputWorkspace",outputSpace);
    alg.setPropertyValue("WorkspaceIndex","2");
    alg.setPropertyValue("StartX","79300");
    alg.setPropertyValue("EndX","79600");

    // create function you want to fit against
    CompositeFunction *fnWithBk = new CompositeFunction();

    LinearBackground *bk = new LinearBackground();
    bk->initialize();

    bk->setParameter("A0",0.0);
    bk->setParameter("A1",0.0);
    bk->removeActive(1);  

    // set up Gaussian fitting function
    Gaussian* fn = new Gaussian();
    fn->initialize();

    // set the workspace explicitely for unit testing purpose only
    Mantid::DataObjects::Workspace2D_sptr wsToPass = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(AnalysisDataService::Instance().retrieve(outputSpace));
    fn->setMatrixWorkspace(wsToPass,2,0,0);
    TS_ASSERT_DELTA( fn->getParameter("Height"), 200 ,0.1);
    TS_ASSERT_DELTA( fn->getParameter("Sigma"), 300 ,0.1);
    IConstraint* testConstraint = fn->getConstraint(2);
    TS_ASSERT( testConstraint->asString().compare("20<Sigma<100") == 0);
    TS_ASSERT_DELTA( testConstraint->getPenaltyFactor(), 1000.001 ,0.00001);

    fnWithBk->addFunction(bk);
    fnWithBk->addFunction(fn);

    alg.setFunction(fnWithBk);

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg.execute() )
    )
    TS_ASSERT( alg.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 5.1604,1);

    TS_ASSERT_DELTA( fn->height(), 232.1146 ,1);
    TS_ASSERT_DELTA( fn->centre(), 79430.1 ,10);
    TS_ASSERT_DELTA( fn->getParameter("Sigma"), 26.14 ,0.1);
    TS_ASSERT_DELTA( bk->getParameter("A0"), 8.0575 ,0.1);
    TS_ASSERT_DELTA( bk->getParameter("A1"), 0.0 ,0.01); 

    AnalysisDataService::Instance().remove(outputSpace);
    InstrumentDataService::Instance().remove(instrumentName);
  }


  // Same as testAgainstHRPD_DatasetWithConstraints but
  // also test <formula> from HRPD_for_UNIT_TESTING2.xml
  void t1estAgainstHRPD_DatasetWithConstraintsTestAlsoFormula()
  {
    // load dataset to test against
    std::string inputFile = "../../../../Test/AutoTestData/HRP38692.raw";
    LoadRaw loader;
    loader.initialize();
    loader.setPropertyValue("Filename", inputFile);
    std::string outputSpace = "HRP38692_Dataset";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    loader.execute();

    // reload instrument to test constraint defined in IDF is working
    LoadInstrument reLoadInstrument;
    reLoadInstrument.initialize();
    std::string instrumentName = "HRPD_for_UNIT_TESTING2.xml";
    reLoadInstrument.setPropertyValue("Filename", "../../../../Test/Instrument/IDFs_for_UNIT_TESTING/" + instrumentName);
    reLoadInstrument.setPropertyValue("Workspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING(reLoadInstrument.execute());
    TS_ASSERT( reLoadInstrument.isExecuted() );

    Fit alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );

    // Set which spectrum to fit against and initial starting values
    alg.setPropertyValue("InputWorkspace",outputSpace);
    alg.setPropertyValue("WorkspaceIndex","2");
    alg.setPropertyValue("StartX","79300");
    alg.setPropertyValue("EndX","79600");

    // create function you want to fit against
    CompositeFunction *fnWithBk = new CompositeFunction();

    LinearBackground *bk = new LinearBackground();
    bk->initialize();

    bk->setParameter("A0",0.0);
    bk->setParameter("A1",0.0);
    bk->removeActive(1);  
    //bk->removeActive(1);

    BoundaryConstraint* bc_b = new BoundaryConstraint(bk,"A0",0, 20.0);
    //bk->addConstraint(bc_b);

    // set up Gaussian fitting function
    Gaussian* fn = new Gaussian();
    fn->initialize();


    // set the workspace explicitely just to make sure that Sigma has been
    // set as expected from the look up table
    Mantid::DataObjects::Workspace2D_sptr wsToPass = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(AnalysisDataService::Instance().retrieve(outputSpace));
    fn->setParameter("PeakCentre",80000.0, false);  // set here for the purpose of the test below only.
    fn->setMatrixWorkspace(wsToPass,2,0,0);
    TS_ASSERT_DELTA( fn->getParameter("Height"), 201.44 ,0.1);

    // add constraint to function
    // BoundaryConstraint* bc3 = new BoundaryConstraint(fn,"Sigma",20, 100.0);  
    // fn->addConstraint(bc3);   // this is set in HRPD_for_UNIT_TESTING.xml

    fnWithBk->addFunction(bk);
    fnWithBk->addFunction(fn);

    alg.setFunction(fnWithBk);

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg.execute() )
    )
    TS_ASSERT( alg.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 5.1604,1);

    TS_ASSERT_DELTA( fn->height(), 232.1146 ,1);
    TS_ASSERT_DELTA( fn->centre(), 79430.1 ,10);
    TS_ASSERT_DELTA( fn->getParameter("Sigma"), 26.14 ,0.1);
    TS_ASSERT_DELTA( bk->getParameter("A0"), 8.0575 ,0.1);
    TS_ASSERT_DELTA( bk->getParameter("A1"), 0.0 ,0.01); 

    AnalysisDataService::Instance().remove(outputSpace);
    InstrumentDataService::Instance().remove(instrumentName);
  }


  void t1estAgainstHRPD_FallbackToSimplex()
  {
    // load dataset to test against
    std::string inputFile = "../../../../Test/AutoTestData/HRP38692.raw";
    LoadRaw loader;
    loader.initialize();
    loader.setPropertyValue("Filename", inputFile);
    std::string outputSpace = "HRPD_Dataset";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    loader.execute();


    Fit alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );

    // Set which spectrum to fit against and initial starting values
    alg.setPropertyValue("InputWorkspace",outputSpace);
    alg.setPropertyValue("WorkspaceIndex","2");
    alg.setPropertyValue("StartX","79300");
    alg.setPropertyValue("EndX","79600");

    // create function you want to fit against
    CompositeFunction *fnWithBk = new CompositeFunction();

    LinearBackground *bk = new LinearBackground();
    bk->initialize();

    bk->setParameter("A0",0.0);
    bk->setParameter("A1",0.0);
    bk->removeActive(1);  
    //bk->removeActive(1);

    BoundaryConstraint* bc_b = new BoundaryConstraint(bk,"A0",0, 20.0);
    bk->addConstraint(bc_b);

    // set up Gaussian fitting function
    Gaussian* fn = new Gaussian();
    fn->initialize();

    fn->setParameter("Height",200.0);   
    fn->setParameter("PeakCentre",79450.0);
    fn->setParameter("Sigma",300.0);

    // add constraint to function
    BoundaryConstraint* bc1 = new BoundaryConstraint(fn,"Height",100, 300.0);
    BoundaryConstraint* bc2 = new BoundaryConstraint(fn,"PeakCentre",79200, 79700.0);
    BoundaryConstraint* bc3 = new BoundaryConstraint(fn,"Sigma",20, 100.0);
    //fn->addConstraint(bc1);
    //fn->addConstraint(bc2);
    //fn->addConstraint(bc3);

    fnWithBk->addFunction(bk);
    fnWithBk->addFunction(fn);

    alg.setFunction(fnWithBk);

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg.execute() )
    )
    TS_ASSERT( alg.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg.getProperty("Output Chi^2/DoF");
    //TS_ASSERT_DELTA( dummy, 5.1604,1);

    TS_ASSERT_DELTA( fn->height(), 249.3187 ,0.01);
    TS_ASSERT_DELTA( fn->centre(), 79430 ,0.1);
    TS_ASSERT_DELTA( fn->getParameter("Sigma"), 25.3066 ,0.01);
    TS_ASSERT_DELTA( bk->getParameter("A0"), 7.8643 ,0.001);
    TS_ASSERT_DELTA( bk->getParameter("A1"), 0.0 ,0.01); 

    AnalysisDataService::Instance().remove(outputSpace);
  }


  void t1estAgainstMockData()
  {
    // create mock data to test against
    std::string wsName = "GaussMockData";
    int histogramNumber = 1;
    int timechannels = 20;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < 20; i++) ws2D->dataX(0)[i] = i+1;
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    getMockData(y, e);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // set up gaussian fitting function
    Gaussian* gaus = new Gaussian();
    gaus->initialize();
    gaus->setCentre(11.2);
    gaus->setHeight(100.7);
    gaus->setWidth(2.2);

    alg2.setFunction(gaus);

    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","0");
    alg2.setPropertyValue("StartX","0");
    alg2.setPropertyValue("EndX","20");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )
    TS_ASSERT( alg2.isExecuted() );

    std::string minimizer = alg2.getProperty("Minimizer");
    TS_ASSERT( minimizer.compare("Levenberg-Marquardt") == 0 );

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 0.0717,0.0001);

    TS_ASSERT_DELTA( gaus->height(), 97.8035 ,0.0001);
    TS_ASSERT_DELTA( gaus->centre(), 11.2356 ,0.0001);
    TS_ASSERT_DELTA( gaus->width(), 2.6237 ,0.0001);

    AnalysisDataService::Instance().remove(wsName);
  }

  void t1estAgainstMockDataSimplex()
  {
    // create mock data to test against
    std::string wsName = "GaussMockDataSimplex";
    int histogramNumber = 1;
    int timechannels = 20;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < 20; i++) ws2D->dataX(0)[i] = i+1;
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    getMockData(y, e);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // set up gaussian fitting function
    SimplexGaussian* gaus = new SimplexGaussian();
    gaus->initialize();
    gaus->setCentre(11.2);
    gaus->setHeight(100.7);
    gaus->setWidth(2.2);

    alg2.setFunction(gaus);

    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","0");
    alg2.setPropertyValue("StartX","0");
    alg2.setPropertyValue("EndX","20");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )
    TS_ASSERT( alg2.isExecuted() );

    std::string minimizer = alg2.getProperty("Minimizer");
    TS_ASSERT( minimizer.compare("Simplex") == 0 );

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 0.0717,0.0001);


    TS_ASSERT_DELTA( gaus->height(), 97.8091 ,0.01);
    TS_ASSERT_DELTA( gaus->centre(), 11.2356 ,0.001);
    TS_ASSERT_DELTA( gaus->width(), 2.6240 ,0.001);

    AnalysisDataService::Instance().remove(wsName);
  }

  void t1estAgainstMockDataSimplex2()
  {
    // create mock data to test against
    std::string wsName = "GaussMockDataSimplex2";
    int histogramNumber = 1;
    int timechannels = 20;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < 20; i++) ws2D->dataX(0)[i] = i+1;
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    getMockData(y, e);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // set up gaussian fitting function
    Gaussian* gaus = new Gaussian();
    gaus->initialize();
    gaus->setCentre(11.2);
    gaus->setHeight(100.7);
    gaus->setWidth(2.2);

    alg2.setFunction(gaus);

    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","0");
    alg2.setPropertyValue("StartX","0");
    alg2.setPropertyValue("EndX","20");
    alg2.setPropertyValue("Minimizer", "Simplex");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )
    TS_ASSERT( alg2.isExecuted() );

    std::string minimizer = alg2.getProperty("Minimizer");
    TS_ASSERT( minimizer.compare("Simplex") == 0 );

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 0.0717,0.0001);


    TS_ASSERT_DELTA( gaus->height(), 97.8091 ,0.01);
    TS_ASSERT_DELTA( gaus->centre(), 11.2356 ,0.001);
    TS_ASSERT_DELTA( gaus->width(), 2.6240 ,0.001);

    AnalysisDataService::Instance().remove(wsName);
  }

  void t1estAgainstMockDataFRConjugateGradient()
  {
    // create mock data to test against
    std::string wsName = "GaussMockDataFRConjugateGradient";
    int histogramNumber = 1;
    int timechannels = 20;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < 20; i++) ws2D->dataX(0)[i] = i+1;
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    getMockData(y, e);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // set up gaussian fitting function
    Gaussian* gaus = new Gaussian();
    gaus->initialize();
    gaus->setCentre(11.2);
    gaus->setHeight(100.7);
    gaus->setWidth(2.2);

    alg2.setFunction(gaus);

    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","0");
    alg2.setPropertyValue("StartX","0");
    alg2.setPropertyValue("EndX","20");
    alg2.setPropertyValue("Minimizer", "Conjugate gradient (Fletcher-Reeves imp.)");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )
    TS_ASSERT( alg2.isExecuted() );

    std::string minimizer = alg2.getProperty("Minimizer");
    TS_ASSERT( minimizer.compare("Conjugate gradient (Fletcher-Reeves imp.)") == 0 );

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 0.0717,0.0001);


    TS_ASSERT_DELTA( gaus->height(), 97.7995 ,0.0001);
    TS_ASSERT_DELTA( gaus->centre(), 11.2356 ,0.001);
    TS_ASSERT_DELTA( gaus->width(), 2.6240 ,0.001);

    AnalysisDataService::Instance().remove(wsName);
  }

  void t1estAgainstMockDataPRConjugateGradient()
  {
    // create mock data to test against
    std::string wsName = "GaussMockDataPRConjugateGradient";
    int histogramNumber = 1;
    int timechannels = 20;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < 20; i++) ws2D->dataX(0)[i] = i+1;
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    getMockData(y, e);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // set up gaussian fitting function
    Gaussian* gaus = new Gaussian();
    gaus->initialize();
    gaus->setCentre(11.2);
    gaus->setHeight(100.7);
    gaus->setWidth(2.2);

    alg2.setFunction(gaus);

    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","0");
    alg2.setPropertyValue("StartX","0");
    alg2.setPropertyValue("EndX","20");
    alg2.setPropertyValue("Minimizer", "Conjugate gradient (Polak-Ribiere imp.)");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )
    TS_ASSERT( alg2.isExecuted() );

    std::string minimizer = alg2.getProperty("Minimizer");
    TS_ASSERT( minimizer.compare("Conjugate gradient (Polak-Ribiere imp.)") == 0 );

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 0.0717,0.0001);


    TS_ASSERT_DELTA( gaus->height(), 97.7857 ,0.0001);
    TS_ASSERT_DELTA( gaus->centre(), 11.2356 ,0.001);
    TS_ASSERT_DELTA( gaus->width(), 2.6240 ,0.001);

    AnalysisDataService::Instance().remove(wsName);
  }

  void t1estAgainstMockDataBFGS()
  {
    // create mock data to test against
    std::string wsName = "GaussMockDataBFGS";
    int histogramNumber = 1;
    int timechannels = 20;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < 20; i++) ws2D->dataX(0)[i] = i+1;
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    getMockData(y, e);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // set up gaussian fitting function
    Gaussian* gaus = new Gaussian();
    gaus->initialize();
    gaus->setCentre(11.2);
    gaus->setHeight(100.7);
    gaus->setWidth(2.2);

    alg2.setFunction(gaus);

    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","0");
    alg2.setPropertyValue("StartX","0");
    alg2.setPropertyValue("EndX","20");
    alg2.setPropertyValue("Minimizer", "BFGS");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )
    TS_ASSERT( alg2.isExecuted() );

    std::string minimizer = alg2.getProperty("Minimizer");
    TS_ASSERT( minimizer.compare("BFGS") == 0 );

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 0.0717,0.0001);


    TS_ASSERT_DELTA( gaus->height(), 97.8111 ,0.0001);
    TS_ASSERT_DELTA( gaus->centre(), 11.2356 ,0.001);
    TS_ASSERT_DELTA( gaus->width(), 2.6240 ,0.001);

    AnalysisDataService::Instance().remove(wsName);
  }


  // here we have an example where an upper constraint on Sigma <= 100 makes
  // the Gaussian fit below success. The starting value of Sigma is here 300. 
  // Note that the fit is equally successful if we had no constraint on Sigma
  // and used a starting of Sigma = 100.
  // Note that the no constraint simplex with Sigma = 300 also does not locate
  // the correct minimum but not as badly as levenberg-marquardt
  void t1estAgainstHRPD_DatasetWithConstraintsSimplex()
  {
    // load dataset to test against
    std::string inputFile = "../../../../Test/AutoTestData/HRP38692.raw";
    LoadRaw loader;
    loader.initialize();
    loader.setPropertyValue("Filename", inputFile);
    std::string outputSpace = "MAR_Dataset";
    loader.setPropertyValue("OutputWorkspace", outputSpace);
    loader.execute();

    //This test will not make sense if the configuration peakRadius is not set correctly.
    const std::string priorRadius = ConfigService::Instance().getString("curvefitting.peakRadius");
    ConfigService::Instance().setString("curvefitting.peakRadius","5");

    Fit alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );

    // Set which spectrum to fit against and initial starting values
    alg.setPropertyValue("InputWorkspace",outputSpace);
    alg.setPropertyValue("WorkspaceIndex","2");
    alg.setPropertyValue("StartX","79300");
    alg.setPropertyValue("EndX","79600");

    // create function you want to fit against
    CompositeFunction *fnWithBk = new CompositeFunction();

    LinearBackground *bk = new LinearBackground();
    bk->initialize();

    bk->setParameter("A0",0.0);
    bk->setParameter("A1",0.0);
    bk->removeActive(1);  
    //bk->removeActive(1);

    BoundaryConstraint* bc_b = new BoundaryConstraint(bk,"A0",0, 20.0);
    //bk->addConstraint(bc_b);

    // set up Gaussian fitting function
    //SimplexGaussian* fn = new SimplexGaussian();
    Gaussian* fn = new Gaussian();
    fn->initialize();

    fn->setParameter("Height",200.0);
    fn->setParameter("PeakCentre",79450.0);
    fn->setParameter("Sigma",10.0);

    // add constraint to function
    BoundaryConstraint* bc1 = new BoundaryConstraint(fn,"Height",100, 300.0);
    BoundaryConstraint* bc2 = new BoundaryConstraint(fn,"PeakCentre",79200, 79700.0);
    BoundaryConstraint* bc3 = new BoundaryConstraint(fn,"Sigma",20, 100.0);
    //fn->addConstraint(bc1);
    //fn->addConstraint(bc2);
    fn->addConstraint(bc3);

    fnWithBk->addFunction(bk);
    fnWithBk->addFunction(fn);

    //alg.setFunction(fnWithBk);
    alg.setPropertyValue("Function",*fnWithBk);
    alg.setPropertyValue("Minimizer","Simplex");

    delete fnWithBk;

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg.execute() )
    )
    TS_ASSERT( alg.isExecuted() );

    std::string minimizer = alg.getProperty("Minimizer");
    TS_ASSERT( minimizer.compare("Simplex") == 0 );

    // test the output from fit is what you expect
    double dummy = alg.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 5.1604,1);

    IFunction* fun = FunctionFactory::Instance().createInitialized(alg.getPropertyValue("Function"));
    TS_ASSERT(fun);
    
    TS_ASSERT_DELTA( fun->getParameter("f1.Height"), 216.419 ,1);
    TS_ASSERT_DELTA( fun->getParameter("f1.PeakCentre"), 79430.1 ,1);
    TS_ASSERT_DELTA( fun->getParameter("f1.Sigma"), 27.08 ,0.1);
    TS_ASSERT_DELTA( fun->getParameter("f0.A0"), 2.18 ,0.1);
    TS_ASSERT_DELTA( fun->getParameter("f0.A1"), 0.0 ,0.01); 

    //TS_ASSERT_DELTA( fn->height(), 232.1146 ,1);
    //TS_ASSERT_DELTA( fn->centre(), 79430.1 ,1);
    //TS_ASSERT_DELTA( fn->getParameter("Sigma"), 26.14 ,0.1);
    //TS_ASSERT_DELTA( bk->getParameter("A0"), 8.0575 ,0.1);
    //TS_ASSERT_DELTA( bk->getParameter("A1"), 0.0 ,0.01); 

    AnalysisDataService::Instance().remove(outputSpace);
    // Be nice and set back to what it was before
    ConfigService::Instance().setString("curvefitting.peakRadius",priorRadius);
  }


};

#endif /*GAUSSIANTEST_H_*/
