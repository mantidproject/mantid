#ifndef MANTID_ALGORITHMS_COPYSAMPLETEST_H_
#define MANTID_ALGORITHMS_COPYSAMPLETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include <iostream>
#include <iomanip>

#include "MantidAlgorithms/CopySample.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidKernel/NeutronAtom.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SampleEnvironment.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Objects/Object.h"

#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::MDEvents;

class CopySampleTest : public CxxTest::TestSuite
{
public:


  void test_Init()
  {
    CopySample alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  Sample createsample()
  {
    Sample sample;
    sample.setName("test");
    const std::string envName("TestKit");
    SampleEnvironment *kit = new SampleEnvironment(envName);
    auto shape = ComponentCreationHelper::createCappedCylinder(0.5, 1.5, V3D(0.0,0.0,0.0), V3D(0.,1.0,0.), "tube");
    kit->add(*shape);
    sample.setEnvironment(kit);
    OrientedLattice *latt = new OrientedLattice(1.0,2.0,3.0, 90, 90, 90);
    sample.setOrientedLattice(latt);
    delete latt;
    Object_sptr shape_sptr = \
      ComponentCreationHelper::createCappedCylinder(0.0127, 1.0, V3D(), V3D(0.0, 1.0, 0.0), "cyl");
    shape_sptr->setMaterial(Material("vanBlock", Mantid::PhysicalConstants::getNeutronAtom(23, 0), 0.072));
    sample.setShape(*shape_sptr);
    return sample;
  }

  void test_exec_all()
  {
    WorkspaceSingleValue_sptr ws1(new WorkspaceSingleValue(1,1));
    WorkspaceSingleValue_sptr ws2(new WorkspaceSingleValue(4,2));
    Sample s=createsample();
    ws1->mutableSample()=s;

    // Name of the output workspace.
    std::string inWSName("CopySampleTest_InputWS");
    std::string outWSName("CopySampleTest_OutputWS");
    AnalysisDataService::Instance().add(inWSName, ws1);
    AnalysisDataService::Instance().add(outWSName, ws2);

    CopySample alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", inWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CopyName", "1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CopyMaterial", "1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CopyEnvironment", "1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CopyShape", "1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CopyLattice", "1") );

    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;

    // Check the results
    Sample copy=ws->mutableSample();
    TS_ASSERT_EQUALS(copy.getName(),"test" );
    TS_ASSERT_EQUALS(copy.getOrientedLattice().c(), 3.0);
    TS_ASSERT_EQUALS(copy.getEnvironment().name(), "TestKit");
    TS_ASSERT_EQUALS(copy.getEnvironment().nelements(), 1);
    TS_ASSERT_DELTA(copy.getMaterial().cohScatterXSection(2.1), 0.0184,  1e-02);
    TS_ASSERT_EQUALS(copy.getShape().getName(),s.getShape().getName());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(inWSName);
    AnalysisDataService::Instance().remove(outWSName);
  }


  void test_exec_some()
  {
    WorkspaceSingleValue_sptr ws1(new WorkspaceSingleValue(1,1));
    WorkspaceSingleValue_sptr ws2(new WorkspaceSingleValue(4,2));
    Sample s=createsample();
    ws1->mutableSample()=s;

    // Name of the output workspace.
    std::string inWSName("CopySampleTest_InputWS");
    std::string outWSName("CopySampleTest_OutputWS");
    AnalysisDataService::Instance().add(inWSName, ws1);
    AnalysisDataService::Instance().add(outWSName, ws2);

    CopySample alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", inWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CopyName", "0") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CopyMaterial", "1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CopyEnvironment", "1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CopyShape", "0") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CopyLattice", "0") );

    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;

    // Check the results
    Sample copy=ws->mutableSample();
    TS_ASSERT_DIFFERS(copy.getName(),"test" );
    TS_ASSERT(!copy.hasOrientedLattice());
    TS_ASSERT_EQUALS(copy.getEnvironment().name(), "TestKit");
    TS_ASSERT_EQUALS(copy.getEnvironment().nelements(), 1);
    TS_ASSERT_DELTA(copy.getMaterial().cohScatterXSection(2.1), 0.0184,  1e-02);
    TS_ASSERT_DIFFERS(copy.getShape().getName(),s.getShape().getName());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(inWSName);
    AnalysisDataService::Instance().remove(outWSName);
  }


  void test_orientation()
  {
    WorkspaceSingleValue_sptr ws1(new WorkspaceSingleValue(1,1));
    WorkspaceSingleValue_sptr ws2(new WorkspaceSingleValue(4,2));
    Sample s=createsample();
    ws1->mutableSample()=s;

    // Name of the output workspace.
    std::string inWSName("CopySampleTest_InputWS");
    std::string outWSName("CopySampleTest_OutputWS");
    AnalysisDataService::Instance().add(inWSName, ws1);
    AnalysisDataService::Instance().add(outWSName, ws2);

    CopySample alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", inWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CopyName", "0") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CopyMaterial", "0") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CopyEnvironment", "0") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CopyShape", "0") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CopyLattice", "1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CopyOrientationOnly", "1") );

    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;

    // Check the results
    Sample copy=ws->mutableSample();
    TS_ASSERT(copy.hasOrientedLattice());
    TS_ASSERT_EQUALS(copy.getOrientedLattice().getUB(), s.getOrientedLattice().getUB());

    //modify the first unit cell, both U and B
    s.getOrientedLattice().setUFromVectors(V3D(1,1,0),V3D(1,-1,0));
    s.getOrientedLattice().seta(1.1);
    ws1->mutableSample()=s;
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    // Retrieve the workspace from data service.
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;

    // Check the results
    copy=ws->mutableSample();
    TS_ASSERT(copy.hasOrientedLattice());
    TS_ASSERT_DIFFERS(copy.getOrientedLattice().a(), s.getOrientedLattice().a()); //different B matrix
    TS_ASSERT_EQUALS(copy.getOrientedLattice().getU(), s.getOrientedLattice().getU());//same U
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(inWSName);
    AnalysisDataService::Instance().remove(outWSName);
  }


  void test_MDcopy()
  {
    IMDEventWorkspace_sptr ew(new MDEventWorkspace<MDEvent<3>, 3>());
    TS_ASSERT_EQUALS( ew->getNumExperimentInfo(), 0);
    ExperimentInfo_sptr ei(new ExperimentInfo);
    ExperimentInfo_sptr ei1(new ExperimentInfo);
    Sample s=createsample();
    Sample s1;
    OrientedLattice *latt=new OrientedLattice(6.0,7.0,8.0, 90, 90, 90);
    s1.setOrientedLattice(latt);
    delete latt;
    s1.setName("newsample");
    ei->mutableSample()=s;
    TS_ASSERT_EQUALS( ew->addExperimentInfo(ei), 0);
    TS_ASSERT_EQUALS( ew->addExperimentInfo(ei), 1);
    ei1->mutableSample()=s1;
    TS_ASSERT_EQUALS( ew->addExperimentInfo(ei1), 2);
    TS_ASSERT_EQUALS( ew->getNumExperimentInfo(), 3);
    TS_ASSERT_EQUALS(ew->getExperimentInfo(1)->sample().getOrientedLattice().c(),3);
    TS_ASSERT_EQUALS(ew->getExperimentInfo(2)->sample().getOrientedLattice().c(),8);

    IMDEventWorkspace_sptr ewout(new MDEventWorkspace<MDEvent<3>, 3>());
    ExperimentInfo_sptr eiout0(new ExperimentInfo);
    eiout0->mutableSample()=s;
    ExperimentInfo_sptr eiout1(new ExperimentInfo);
    ExperimentInfo_sptr eiout2(new ExperimentInfo);
    ExperimentInfo_sptr eiout3(new ExperimentInfo);
    TS_ASSERT_EQUALS( ewout->addExperimentInfo(eiout0), 0);
    TS_ASSERT_EQUALS( ewout->addExperimentInfo(eiout1), 1);
    TS_ASSERT_EQUALS( ewout->addExperimentInfo(eiout2), 2);
    TS_ASSERT_EQUALS( ewout->addExperimentInfo(eiout3), 3);
    TS_ASSERT(ewout->getExperimentInfo(0)->sample().hasOrientedLattice());
    TS_ASSERT(!ewout->getExperimentInfo(1)->sample().hasOrientedLattice());
    TS_ASSERT(!ewout->getExperimentInfo(2)->sample().hasOrientedLattice());
    TS_ASSERT(!ewout->getExperimentInfo(3)->sample().hasOrientedLattice());

    //run algorithm twice: set all samples to s1, then set sample in last experiment info to s
    std::string inWSName("CopySampleTest_InputWS");
    std::string outWSName("CopySampleTest_OutputWS");
    AnalysisDataService::Instance().add(inWSName, ew);
    AnalysisDataService::Instance().add(outWSName, ewout);
    CopySample alg,alg1;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", inWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CopyName", "1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CopyMaterial", "0") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CopyEnvironment", "0") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CopyShape", "0") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CopyLattice", "1") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("MDInputSampleNumber", "2") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("MDOutputSampleNumber", "-1") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    TS_ASSERT_THROWS_NOTHING( alg1.initialize() )
    TS_ASSERT( alg1.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("InputWorkspace", inWSName) );
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("CopyName", "1") );
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("CopyMaterial", "0") );
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("CopyEnvironment", "0") );
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("CopyShape", "0") );
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("CopyLattice", "1") );
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("MDInputSampleNumber", "0") );
    TS_ASSERT_THROWS_NOTHING( alg1.setPropertyValue("MDOutputSampleNumber", "3") );
    TS_ASSERT_THROWS_NOTHING( alg1.execute(); );
    TS_ASSERT( alg1.isExecuted() );

    // Retrieve the workspace from data service.
    IMDEventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;

    // test output
    TS_ASSERT(ws->getExperimentInfo(0)->sample().hasOrientedLattice());
    TS_ASSERT(ws->getExperimentInfo(1)->sample().hasOrientedLattice());
    TS_ASSERT(ws->getExperimentInfo(2)->sample().hasOrientedLattice());
    TS_ASSERT(ws->getExperimentInfo(3)->sample().hasOrientedLattice());
    TS_ASSERT_EQUALS(ws->getExperimentInfo(0)->sample().getOrientedLattice().a(),6);
    TS_ASSERT_EQUALS(ws->getExperimentInfo(1)->sample().getOrientedLattice().c(),8);
    TS_ASSERT_EQUALS(ws->getExperimentInfo(2)->sample().getOrientedLattice().c(),8);
    TS_ASSERT_EQUALS(ws->getExperimentInfo(3)->sample().getOrientedLattice().c(),3);
    TS_ASSERT_EQUALS(ws->getExperimentInfo(0)->sample().getName(),"newsample");
    TS_ASSERT_EQUALS(ws->getExperimentInfo(1)->sample().getName(),"newsample");
    TS_ASSERT_EQUALS(ws->getExperimentInfo(2)->sample().getName(),"newsample");
    TS_ASSERT_EQUALS(ws->getExperimentInfo(3)->sample().getName(),"test");
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(inWSName);
    AnalysisDataService::Instance().remove(outWSName);
  }


};


#endif /* MANTID_ALGORITHMS_COPYSAMPLETEST_H_ */

