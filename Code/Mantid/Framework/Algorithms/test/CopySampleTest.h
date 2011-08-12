#ifndef MANTID_ALGORITHMS_COPYSAMPLETEST_H_
#define MANTID_ALGORITHMS_COPYSAMPLETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
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

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

class CopySampleTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    CopySample alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  Object_sptr createCappedCylinder(double radius, double height, const V3D & baseCentre, const V3D & axis, const std::string & id)
  {
    std::ostringstream xml;
    xml << "<cylinder id=\"" << id << "\">"
      << "<centre-of-bottom-base x=\"" << baseCentre.X() << "\" y=\"" << baseCentre.Y() << "\" z=\"" << baseCentre.Z() << "\"/>"
      << "<axis x=\"" << axis.X() << "\" y=\"" << axis.Y() << "\" z=\"" << axis.Z() << "\"/>"
      << "<radius val=\"" << radius << "\" />"
      << "<height val=\"" << height << "\" />"  << "</cylinder>";
    ShapeFactory shapeMaker;
    return shapeMaker.createShape(xml.str());
  }
  ObjComponent * createSingleObjectComponent()
  {
    Object_sptr pixelShape = createCappedCylinder(0.5, 1.5, V3D(0.0,0.0,0.0), V3D(0.,1.0,0.), "tube");
    return new ObjComponent("pixel", pixelShape);
  }

  Sample createsample()
  {
    Sample sample;
    sample.setName("test");
    const std::string envName("TestKit");
    SampleEnvironment *kit = new SampleEnvironment(envName);
    kit->add(createSingleObjectComponent());
    sample.setEnvironment(kit);
    OrientedLattice *latt = new OrientedLattice(1.0,2.0,3.0, 90, 90, 90);
    sample.setOrientedLattice(latt);
    Material *vanBlock = new Material("vanBlock", Mantid::PhysicalConstants::getNeutronAtom(23, 0), 0.072);
    sample.setMaterial(*vanBlock);
    Object_sptr shape_sptr =
      createCappedCylinder(0.0127, 1.0, V3D(), V3D(0.0, 1.0, 0.0), "cyl");
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
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(ws);
    if (!ws) return;

    // Check the results
    Sample copy=ws->mutableSample();
    TS_ASSERT_EQUALS(copy.getName(),"test" );
    TS_ASSERT_EQUALS(copy.getOrientedLattice().c(), 3.0);
    TS_ASSERT_EQUALS(copy.getEnvironment().getName(), "TestKit");
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
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(ws);
    if (!ws) return;

    // Check the results
    Sample copy=ws->mutableSample();
    TS_ASSERT_DIFFERS(copy.getName(),"test" );
    TS_ASSERT(!copy.hasOrientedLattice());
    TS_ASSERT_EQUALS(copy.getEnvironment().getName(), "TestKit");
    TS_ASSERT_EQUALS(copy.getEnvironment().nelements(), 1);
    TS_ASSERT_DELTA(copy.getMaterial().cohScatterXSection(2.1), 0.0184,  1e-02);
    TS_ASSERT_DIFFERS(copy.getShape().getName(),s.getShape().getName());

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(inWSName);
    AnalysisDataService::Instance().remove(outWSName);
  }
  void test_MDcopy()
  {
  }


};


#endif /* MANTID_ALGORITHMS_COPYSAMPLETEST_H_ */

