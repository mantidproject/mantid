#ifndef MANTID_MDALGORITHMS_CONVERTTOMDHELPER2TEST_H_
#define MANTID_MDALGORITHMS_CONVERTTOMDHELPER2TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/ConvertToMDMinMaxLocal.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/TimeSeriesProperty.h"



using Mantid::MDAlgorithms::ConvertToMDMinMaxLocal;
class ConvertToMDMinMaxLocalTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertToMDMinMaxLocalTest *createSuite() { return new ConvertToMDMinMaxLocalTest(); }
  static void destroySuite( ConvertToMDMinMaxLocalTest *suite ) { delete suite; }

  ConvertToMDMinMaxLocalTest():WSName("CMDHTest")
  {

  }


  void test_Init()
  {
    ConvertToMDMinMaxLocal alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_direct1D()
  {

    Mantid::API::FrameworkManager::Instance();
    ConvertToMDMinMaxLocal alg;
    Mantid::API::MatrixWorkspace_sptr  ws=MakeWorkspace(-50,1,true,60,0);
    WorkspaceCreationHelper::storeWS(WSName,ws);
    
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", WSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("QDimensions","|Q|") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("dEAnalysisMode","Direct") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    // Check the results
    // this is spherical instrument
    //TS_ASSERT_EQUALS(alg.getPropertyValue("MinValues"),"0.,-50");
    //TS_ASSERT_EQUALS(alg.getPropertyValue("MaxValues"),"12.667,50");
    // real instrument produces as follows:
    TS_ASSERT_EQUALS("0.535435,-50",alg.getPropertyValue("MinValues"));
    TS_ASSERT_EQUALS("3.20262,50",alg.getPropertyValue("MaxValues"));
    // Remove workspace from the data service.
    Mantid::API::AnalysisDataService::Instance().remove(WSName);
  }

  void test_direct3D()
  {

    Mantid::API::FrameworkManager::Instance();
    ConvertToMDMinMaxLocal alg;
    Mantid::API::MatrixWorkspace_sptr  ws=MakeWorkspace(-50,1,true,60,0);
    WorkspaceCreationHelper::storeWS(WSName,ws);

    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", WSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("QDimensions","Q3D") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("dEAnalysisMode","Direct") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("QConversionScales","Q in A^-1") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    // Check the results
    double minS[]={-0.724983,-0,-1.86877,-50};
    double maxS[]={-0.218591,-0,3.19515,50};
    std::vector<double> minSample(minS,minS+4),maxSample(maxS,maxS+4);

    std::vector<double> minVal = alg.getProperty("MinValues");
    std::vector<double> maxVal = alg.getProperty("MaxValues");
    for(size_t i=0;i<4;i++)
    {
      TS_ASSERT_DELTA(minSample[i],minVal[i],1.e-4);
      TS_ASSERT_DELTA(maxSample[i],maxVal[i],1.e-4);
    }

    // Remove workspace from the data service.
    Mantid::API::AnalysisDataService::Instance().remove(WSName);
  }
  
  void test_direct3DHKL()
  {

    Mantid::API::FrameworkManager::Instance();
    ConvertToMDMinMaxLocal alg;
    Mantid::API::MatrixWorkspace_sptr  ws=MakeWorkspace(-50,1,true,60,0);
    WorkspaceCreationHelper::storeWS(WSName,ws);

    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", WSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("QDimensions","Q3D") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("dEAnalysisMode","Direct") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("QConversionScales","HKL") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Check the results
    double minS[]={-0.724983,-0,-1.86877,-50};
    double maxS[]={-0.218591,-0,3.19515,50};
    std::vector<double> minSample(minS,minS+4),maxSample(maxS,maxS+4);

    std::vector<double> minVal = alg.getProperty("MinValues");
    std::vector<double> maxVal = alg.getProperty("MaxValues");
    for(size_t i=0;i<4;i++)
    {
      TS_ASSERT_DELTA(minSample[i],minVal[i],1.e-4);
      TS_ASSERT_DELTA(maxSample[i],maxVal[i],1.e-4);
    }


    // Remove workspace from the data service.
    Mantid::API::AnalysisDataService::Instance().remove(WSName);
  }

  void test_indirect1D()
  {
    
    Mantid::API::FrameworkManager::Instance();
    ConvertToMDMinMaxLocal alg;
    Mantid::API::MatrixWorkspace_sptr  ws=MakeWorkspace(-2.5,0.05,true,0,5);
    WorkspaceCreationHelper::storeWS(WSName,ws);

    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", WSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("QDimensions","|Q|") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("dEAnalysisMode","Indirect") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    // Check the results

    TS_ASSERT_EQUALS("0.53808,-2.5",alg.getPropertyValue("MinValues"));
    TS_ASSERT_EQUALS("0.55285,2.5",alg.getPropertyValue("MaxValues"));
    // Remove workspace from the data service.
    Mantid::API::AnalysisDataService::Instance().remove(WSName);
  }

  void test_elastic1D()
  {

    Mantid::API::FrameworkManager::Instance();
    ConvertToMDMinMaxLocal alg;
    Mantid::API::MatrixWorkspace_sptr  ws=MakeWorkspace(25000,10,false,0,0);
    WorkspaceCreationHelper::storeWS(WSName,ws);

    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", WSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("QDimensions","|Q|") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("dEAnalysisMode","Elastic") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    // Check the results

    TS_ASSERT_EQUALS(alg.getPropertyValue("MinValues"),"0.12187");
    TS_ASSERT_EQUALS(alg.getPropertyValue("MaxValues"),"0.126745");
    // Remove workspace from the data service.
    Mantid::API::AnalysisDataService::Instance().remove(WSName);
  }

  void test_elastic1DandExtra()
  {

    Mantid::API::FrameworkManager::Instance();
    ConvertToMDMinMaxLocal alg;
    Mantid::API::MatrixWorkspace_sptr  ws=MakeWorkspace(25000,10,false,0,0);
    WorkspaceCreationHelper::storeWS(WSName,ws);

    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", WSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("QDimensions","|Q|") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("dEAnalysisMode","Elastic") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OtherDimensions","doubleProp") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    // Check the results

    TS_ASSERT_EQUALS(alg.getPropertyValue("MinValues"),"0.12187,9.99");
    TS_ASSERT_EQUALS(alg.getPropertyValue("MaxValues"),"0.126745,9.99");
    // Remove workspace from the data service.
    Mantid::API::AnalysisDataService::Instance().remove(WSName);
  }

private:
  std::string WSName;


  Mantid::API::MatrixWorkspace_sptr MakeWorkspace(double xmin,double dx,bool deltaEUnits,
                                                  double Ei, double Ef)
  {

      Mantid::API::MatrixWorkspace_sptr  ws= WorkspaceCreationHelper::Create2DWorkspaceBinned(1,100,xmin, dx);

      if((Ei>0 || Ef>0)&&deltaEUnits)
      {
          ws->getAxis(0)->setUnit("DeltaE");
      }
      else
      {
          ws->getAxis(0)->setUnit("TOF");
      }

      Mantid::Geometry::Instrument_sptr testInst(new Mantid::Geometry::Instrument);
      ws->setInstrument(testInst);
      // Define a source and sample position
      //Define a source component
      Mantid::Geometry::ObjComponent *source = new Mantid::Geometry::ObjComponent("moderator", Mantid::Geometry::Object_sptr(), testInst.get());
      source->setPos(Mantid::Kernel::V3D(0, 0.0, -15.));
      testInst->add(source);
      testInst->markAsSource(source);
      // Define a sample as a simple sphere
      Mantid::Geometry::ObjComponent *sample = new Mantid::Geometry::ObjComponent("samplePos", Mantid::Geometry::Object_sptr(), testInst.get());
      testInst->setPos(0.0, 0.0, 0.0);
      testInst->add(sample);
      testInst->markAsSamplePos(sample);
      //Detector
      Mantid::Geometry::Detector * physicalPixel = new Mantid::Geometry::Detector("pixel", 1, testInst.get());
      physicalPixel->setPos(0.5,0,5.0);
      testInst->add(physicalPixel);
      testInst->markAsDetector(physicalPixel);

      ws->getSpectrum(0)->addDetectorID(physicalPixel->getID());


      if (Ei>0)
      {
          ws->mutableRun().addProperty(new Mantid::Kernel::PropertyWithValue<double>("Ei",Ei));
      }

      if (Ef>0)
      {
          Mantid::Geometry::ParameterMap pmap(ws->instrumentParameters());
          pmap.addDouble(physicalPixel,"Efixed",Ef);
          ws->replaceInstrumentParameters(pmap);
      }
      Mantid::Geometry::OrientedLattice latt(2,3,4,90,90,90);
      ws->mutableSample().setOrientedLattice(&latt);

      Mantid::Kernel::TimeSeriesProperty<double> * p = new Mantid::Kernel::TimeSeriesProperty<double>("doubleProp");
      TS_ASSERT_THROWS_NOTHING( p->addValue("2007-11-30T16:17:00",9.99) );
      TS_ASSERT_THROWS_NOTHING( p->addValue("2007-11-30T16:17:10",7.55) );
      TS_ASSERT_THROWS_NOTHING( p->addValue("2007-11-30T16:17:20",5.55) );
      TS_ASSERT_THROWS_NOTHING( p->addValue("2007-11-30T16:17:30",10.55) );

      ws->mutableRun().addLogData(p);

      return ws;
  }
};


#endif /* MANTID_MDALGORITHMS_CONVERTTOMDHELPERTEST_H_ */
