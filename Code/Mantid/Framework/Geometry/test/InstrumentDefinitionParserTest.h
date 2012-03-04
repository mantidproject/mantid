#ifndef MANTID_GEOMETRY_INSTRUMENTDEFINITIONPARSERTEST_H_
#define MANTID_GEOMETRY_INSTRUMENTDEFINITIONPARSERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidGeometry/Instrument/InstrumentDefinitionParser.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using Mantid::Kernel::ConfigService;
using Mantid::Kernel::Strings::loadFile;

class InstrumentDefinitionParserTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentDefinitionParserTest *createSuite() { return new InstrumentDefinitionParserTest(); }
  static void destroySuite( InstrumentDefinitionParserTest *suite ) { delete suite; }

  void test_extract_ref_info() 
  {
    std::string filename = ConfigService::Instance().getInstrumentDirectory() + "/IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING.xml";
    std::string xmlText = Strings::loadFile(filename);
    boost::shared_ptr<const Instrument> i;

    // Parse the XML
    InstrumentDefinitionParser parser;
    TS_ASSERT_THROWS_NOTHING( parser.initialize(filename, "For Unit Testing", xmlText); );
    TS_ASSERT_THROWS_NOTHING( i = parser.parseXML(NULL); );

    // Extract the reference frame object
    boost::shared_ptr<const ReferenceFrame> frame = i->getReferenceFrame();

    // Test that values have been populated with expected values (those from file).
    TS_ASSERT_EQUALS(Right, frame->getHandedness());
    TS_ASSERT_EQUALS(Y, frame->pointingUp());
    TS_ASSERT_EQUALS(Z, frame->pointingAlongBeam());
    TS_ASSERT(frame->origin().empty());
  }


  void test_parse_IDF_for_unit_testing() // IDF stands for Instrument Definition File
  {
    std::string filename = ConfigService::Instance().getInstrumentDirectory() + "/IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING.xml";
    std::string xmlText = Strings::loadFile(filename);
    boost::shared_ptr<const Instrument> i;

    // Parse the XML
    InstrumentDefinitionParser parser;
    TS_ASSERT_THROWS_NOTHING( parser.initialize(filename, "For Unit Testing", xmlText); );
    TS_ASSERT_THROWS_NOTHING( i = parser.parseXML(NULL); );

    // Check the mangled name
    TS_ASSERT_EQUALS( parser.getMangledName(), "IDF_for_UNIT_TESTING.xmlHello!");

    boost::shared_ptr<const IObjComponent> source = i->getSource();
    TS_ASSERT_EQUALS( source->getName(), "undulator");
    TS_ASSERT_DELTA( source->getPos().Z(), -17.0,0.01);

    boost::shared_ptr<const IObjComponent> samplepos = i->getSample();
    TS_ASSERT_EQUALS( samplepos->getName(), "nickel-holder");
    TS_ASSERT_DELTA( samplepos->getPos().Y(), 0.0,0.01);

    boost::shared_ptr<const IDetector> ptrDet1 = i->getDetector(1);
    TS_ASSERT_EQUALS( ptrDet1->getID(), 1);
    TS_ASSERT_DELTA( ptrDet1->getPos().X(),  0.0, 0.0001);
    TS_ASSERT_DELTA( ptrDet1->getPos().Y(), 10.0, 0.0001);
    TS_ASSERT_DELTA( ptrDet1->getPos().Z(),  0.0, 0.0001);
    double d = ptrDet1->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d,10.0,0.0001);
    double cmpDistance = ptrDet1->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance,10.0,0.0001);

    boost::shared_ptr<const IDetector> ptrDet2 = i->getDetector(2);
    TS_ASSERT_EQUALS( ptrDet2->getID(), 2);
    TS_ASSERT_DELTA( ptrDet2->getPos().X(),  0.0, 0.0001);
    TS_ASSERT_DELTA( ptrDet2->getPos().Y(), -10.0, 0.0001);
    TS_ASSERT_DELTA( ptrDet2->getPos().Z(),  0.0, 0.0001);
    d = ptrDet2->getPos().distance(samplepos->getPos());
    TS_ASSERT_DELTA(d,10.0,0.0001);
    cmpDistance = ptrDet2->getDistance(*samplepos);
    TS_ASSERT_DELTA(cmpDistance,10.0,0.0001);


    // test if detectors face sample
    TS_ASSERT( !ptrDet1->isValid(V3D(0.02,0.0,0.0)+ptrDet1->getPos()) );
    TS_ASSERT( !ptrDet1->isValid(V3D(-0.02,0.0,0.0)+ptrDet1->getPos()) );
    TS_ASSERT( ptrDet1->isValid(V3D(0.0,0.02,0.0)+ptrDet1->getPos()) );
    TS_ASSERT( !ptrDet1->isValid(V3D(0.0,-0.02,0.0)+ptrDet1->getPos()) );
    TS_ASSERT( !ptrDet1->isValid(V3D(0.0,0.0,0.02)+ptrDet1->getPos()) );
    TS_ASSERT( !ptrDet1->isValid(V3D(0.0,0.0,-0.02)+ptrDet1->getPos()) );

    TS_ASSERT( !ptrDet2->isValid(V3D(0.02,0.0,0.0)+ptrDet2->getPos()) );
    TS_ASSERT( !ptrDet2->isValid(V3D(-0.02,0.0,0.0)+ptrDet2->getPos()) );
    TS_ASSERT( !ptrDet2->isValid(V3D(0.0,0.02,0.0)+ptrDet2->getPos()) );
    TS_ASSERT( ptrDet2->isValid(V3D(0.0,-0.02,0.0)+ptrDet2->getPos()) );
    TS_ASSERT( !ptrDet2->isValid(V3D(0.0,0.0,0.02)+ptrDet2->getPos()) );
    TS_ASSERT( !ptrDet2->isValid(V3D(0.0,0.0,-0.02)+ptrDet2->getPos()) );

    boost::shared_ptr<const IDetector> ptrDet3 = i->getDetector(3);
    TS_ASSERT( !ptrDet3->isValid(V3D(0.02,0.0,0.0)+ptrDet3->getPos()) );
    TS_ASSERT( !ptrDet3->isValid(V3D(-0.02,0.0,0.0)+ptrDet3->getPos()) );
    TS_ASSERT( !ptrDet3->isValid(V3D(0.0,0.02,0.0)+ptrDet3->getPos()) );
    TS_ASSERT( !ptrDet3->isValid(V3D(0.0,-0.02,0.0)+ptrDet3->getPos()) );
    TS_ASSERT( ptrDet3->isValid(V3D(0.0,0.0,0.02)+ptrDet3->getPos()) );
    TS_ASSERT( !ptrDet3->isValid(V3D(0.0,0.0,-0.02)+ptrDet3->getPos()) );

    boost::shared_ptr<const IDetector> ptrDet4 = i->getDetector(4);
    TS_ASSERT( !ptrDet4->isValid(V3D(0.02,0.0,0.0)+ptrDet4->getPos()) );
    TS_ASSERT( !ptrDet4->isValid(V3D(-0.02,0.0,0.0)+ptrDet4->getPos()) );
    TS_ASSERT( !ptrDet4->isValid(V3D(0.0,0.02,0.0)+ptrDet4->getPos()) );
    TS_ASSERT( !ptrDet4->isValid(V3D(0.0,-0.02,0.0)+ptrDet4->getPos()) );
    TS_ASSERT( !ptrDet4->isValid(V3D(0.0,0.0,0.02)+ptrDet4->getPos()) );
    TS_ASSERT( ptrDet4->isValid(V3D(0.0,0.0,-0.02)+ptrDet4->getPos()) );

    // test of facing as a sub-element of location
    boost::shared_ptr<const IDetector> ptrDet5 = i->getDetector(5);
    TS_ASSERT( !ptrDet5->isValid(V3D(0.02,0.0,0.0)+ptrDet5->getPos()) );
    TS_ASSERT( ptrDet5->isValid(V3D(-0.02,0.0,0.0)+ptrDet5->getPos()) );
    TS_ASSERT( !ptrDet5->isValid(V3D(0.0,0.02,0.0)+ptrDet5->getPos()) );
    TS_ASSERT( !ptrDet5->isValid(V3D(0.0,-0.02,0.0)+ptrDet5->getPos()) );
    TS_ASSERT( !ptrDet5->isValid(V3D(0.0,0.0,0.02)+ptrDet5->getPos()) );
    TS_ASSERT( !ptrDet5->isValid(V3D(0.0,0.0,-0.02)+ptrDet5->getPos()) );

    // test of infinite-cone.
    boost::shared_ptr<const IDetector> ptrDet6 = i->getDetector(6);
    TS_ASSERT( !ptrDet6->isValid(V3D(0.02,0.0,0.0)+ptrDet6->getPos()) );
    TS_ASSERT( !ptrDet6->isValid(V3D(-0.02,0.0,0.0)+ptrDet6->getPos()) );
    TS_ASSERT( !ptrDet6->isValid(V3D(0.0,0.02,0.0)+ptrDet6->getPos()) );
    TS_ASSERT( !ptrDet6->isValid(V3D(0.0,-0.02,0.0)+ptrDet6->getPos()) );
    TS_ASSERT( !ptrDet6->isValid(V3D(0.0,0.0,0.02)+ptrDet6->getPos()) );
    TS_ASSERT( ptrDet6->isValid(V3D(0.0,0.0,-0.02)+ptrDet6->getPos()) );
    TS_ASSERT( ptrDet6->isValid(V3D(0.0,0.0,-1.02)+ptrDet6->getPos()) );

    // test of (finite) cone.
    boost::shared_ptr<const IDetector> ptrDet7 = i->getDetector(7);
    TS_ASSERT( !ptrDet7->isValid(V3D(0.02,0.0,0.0)+ptrDet7->getPos()) );
    TS_ASSERT( !ptrDet7->isValid(V3D(-0.02,0.0,0.0)+ptrDet7->getPos()) );
    TS_ASSERT( !ptrDet7->isValid(V3D(0.0,0.02,0.0)+ptrDet7->getPos()) );
    TS_ASSERT( !ptrDet7->isValid(V3D(0.0,-0.02,0.0)+ptrDet7->getPos()) );
    TS_ASSERT( !ptrDet7->isValid(V3D(0.0,0.0,0.02)+ptrDet7->getPos()) );
    TS_ASSERT( ptrDet7->isValid(V3D(0.0,0.0,-0.02)+ptrDet7->getPos()) );
    TS_ASSERT( !ptrDet7->isValid(V3D(0.0,0.0,-1.02)+ptrDet7->getPos()) );

    // test of hexahedron.
    boost::shared_ptr<const IDetector> ptrDet8 = i->getDetector(8);
    TS_ASSERT( ptrDet8->isValid(V3D(0.4,0.4,0.0)+ptrDet8->getPos()) );
    TS_ASSERT( ptrDet8->isValid(V3D(0.8,0.8,0.0)+ptrDet8->getPos()) );
    TS_ASSERT( ptrDet8->isValid(V3D(0.4,0.4,2.0)+ptrDet8->getPos()) );
    TS_ASSERT( !ptrDet8->isValid(V3D(0.8,0.8,2.0)+ptrDet8->getPos()) );
    TS_ASSERT( !ptrDet8->isValid(V3D(0.0,0.0,-0.02)+ptrDet8->getPos()) );
    TS_ASSERT( !ptrDet8->isValid(V3D(0.0,0.0,2.02)+ptrDet8->getPos()) );
    TS_ASSERT( ptrDet8->isValid(V3D(0.5,0.5,0.1)+ptrDet8->getPos()) );

    // test for "cuboid-rotating-test".
    boost::shared_ptr<const IDetector> ptrDet10 = i->getDetector(10);
    TS_ASSERT( ptrDet10->isValid(V3D(0.0,0.0,0.1)+ptrDet10->getPos()) );
    TS_ASSERT( ptrDet10->isValid(V3D(0.0,0.0,-0.1)+ptrDet10->getPos()) );
    TS_ASSERT( ptrDet10->isValid(V3D(0.0,0.02,0.1)+ptrDet10->getPos()) );
    TS_ASSERT( ptrDet10->isValid(V3D(0.0,0.02,-0.1)+ptrDet10->getPos()) );
    TS_ASSERT( !ptrDet10->isValid(V3D(0.0,0.05,0.0)+ptrDet10->getPos()) );
    TS_ASSERT( !ptrDet10->isValid(V3D(0.0,-0.05,0.0)+ptrDet10->getPos()) );
    TS_ASSERT( !ptrDet10->isValid(V3D(0.0,-0.01,0.05)+ptrDet10->getPos()) );
    TS_ASSERT( !ptrDet10->isValid(V3D(0.0,-0.01,-0.05)+ptrDet10->getPos()) );
    boost::shared_ptr<const IDetector> ptrDet11 = i->getDetector(11);
    TS_ASSERT( ptrDet11->isValid(V3D(-0.07,0.0,-0.07)+ptrDet11->getPos()) );
    TS_ASSERT( ptrDet11->isValid(V3D(0.07,0.0,0.07)+ptrDet11->getPos()) );
    TS_ASSERT( ptrDet11->isValid(V3D(0.07,0.01,0.07)+ptrDet11->getPos()) );
    TS_ASSERT( ptrDet11->isValid(V3D(-0.07,0.01,-0.07)+ptrDet11->getPos()) );
    TS_ASSERT( !ptrDet11->isValid(V3D(0.0,0.05,0.0)+ptrDet11->getPos()) );
    TS_ASSERT( !ptrDet11->isValid(V3D(0.0,-0.05,0.0)+ptrDet11->getPos()) );
    TS_ASSERT( !ptrDet11->isValid(V3D(0.0,-0.01,0.05)+ptrDet11->getPos()) );
    TS_ASSERT( !ptrDet11->isValid(V3D(0.0,-0.01,-0.05)+ptrDet11->getPos()) );

    // test for "infinite-cylinder-test".
    boost::shared_ptr<const IDetector> ptrDet12 = i->getDetector(12);
    TS_ASSERT( ptrDet12->isValid(V3D(0.0,0.0,0.1)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(0.0,0.0,-0.1)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(0.0,0.1,0.0)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(0.0,-0.1,0.0)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(0.1,0.0,0.0)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(-0.1,0.0,0.0)+ptrDet12->getPos()) );
    TS_ASSERT( ptrDet12->isValid(V3D(0.0,0.0,0.0)+ptrDet12->getPos()) );
    TS_ASSERT( !ptrDet12->isValid(V3D(2.0,0.0,0.0)+ptrDet12->getPos()) );

    // test for "finite-cylinder-test".
    boost::shared_ptr<const IDetector> ptrDet13 = i->getDetector(13);
    TS_ASSERT( ptrDet13->isValid(V3D(0.0,0.0,0.1)+ptrDet13->getPos()) );
    TS_ASSERT( !ptrDet13->isValid(V3D(0.0,0.0,-0.1)+ptrDet13->getPos()) );
    TS_ASSERT( ptrDet13->isValid(V3D(0.0,0.1,0.0)+ptrDet13->getPos()) );
    TS_ASSERT( ptrDet13->isValid(V3D(0.0,-0.1,0.0)+ptrDet13->getPos()) );
    TS_ASSERT( ptrDet13->isValid(V3D(0.1,0.0,0.0)+ptrDet13->getPos()) );
    TS_ASSERT( ptrDet13->isValid(V3D(-0.1,0.0,0.0)+ptrDet13->getPos()) );
    TS_ASSERT( ptrDet13->isValid(V3D(0.0,0.0,0.0)+ptrDet13->getPos()) );
    TS_ASSERT( !ptrDet13->isValid(V3D(2.0,0.0,0.0)+ptrDet13->getPos()) );

    // test for "complement-test".
    boost::shared_ptr<const IDetector> ptrDet14 = i->getDetector(14);
    TS_ASSERT( !ptrDet14->isValid(V3D(0.0,0.0,0.0)+ptrDet14->getPos()) );
    TS_ASSERT( !ptrDet14->isValid(V3D(0.0,0.0,-0.04)+ptrDet14->getPos()) );
    TS_ASSERT( ptrDet14->isValid(V3D(0.0,0.0,-0.06)+ptrDet14->getPos()) );
    TS_ASSERT( !ptrDet14->isValid(V3D(0.0,0.04,0.0)+ptrDet14->getPos()) );
    TS_ASSERT( ptrDet14->isValid(V3D(0.0,0.06,0.0)+ptrDet14->getPos()) );
    TS_ASSERT( !ptrDet14->isValid(V3D(0.06,0.0,0.0)+ptrDet14->getPos()) );
    TS_ASSERT( !ptrDet14->isValid(V3D(0.51,0.0,0.0)+ptrDet14->getPos()) );
    TS_ASSERT( !ptrDet14->isValid(V3D(0.0,0.51,0.0)+ptrDet14->getPos()) );
    TS_ASSERT( !ptrDet14->isValid(V3D(0.0,0.0,0.51)+ptrDet14->getPos()) );

    // test for "rotation-of-element-test".
    boost::shared_ptr<const IDetector> ptrDet15 = i->getDetector(15);
    TS_ASSERT( !ptrDet15->isValid(V3D(0.0,0.09,0.01)+ptrDet15->getPos()) );
    TS_ASSERT( !ptrDet15->isValid(V3D(0.0,-0.09,0.01)+ptrDet15->getPos()) );
    TS_ASSERT( ptrDet15->isValid(V3D(0.09,0.0,0.01)+ptrDet15->getPos()) );
    TS_ASSERT( ptrDet15->isValid(V3D(-0.09,0.0,0.01)+ptrDet15->getPos()) );
    boost::shared_ptr<const IDetector> ptrDet16 = i->getDetector(16);
    TS_ASSERT( ptrDet16->isValid(V3D(0.0,0.0,0.09)+ptrDet16->getPos()) );
    TS_ASSERT( ptrDet16->isValid(V3D(0.0,0.0,-0.09)+ptrDet16->getPos()) );
    TS_ASSERT( !ptrDet16->isValid(V3D(0.0,0.09,0.0)+ptrDet16->getPos()) );
    TS_ASSERT( !ptrDet16->isValid(V3D(0.0,0.09,0.0)+ptrDet16->getPos()) );
    boost::shared_ptr<const IDetector> ptrDet17 = i->getDetector(17);
    TS_ASSERT( ptrDet17->isValid(V3D(0.0,0.09,0.01)+ptrDet17->getPos()) );
    TS_ASSERT( ptrDet17->isValid(V3D(0.0,-0.09,0.01)+ptrDet17->getPos()) );
    TS_ASSERT( !ptrDet17->isValid(V3D(0.09,0.0,0.01)+ptrDet17->getPos()) );
    TS_ASSERT( !ptrDet17->isValid(V3D(-0.09,0.0,0.01)+ptrDet17->getPos()) );

    // test of sample shape
    TS_ASSERT( samplepos->isValid(V3D(0.0,0.0,0.005)+samplepos->getPos()) );
    TS_ASSERT( !samplepos->isValid(V3D(0.0,0.0,0.05)+samplepos->getPos()) );
    TS_ASSERT( samplepos->isValid(V3D(10.0,0.0,0.005)+samplepos->getPos()) );
    TS_ASSERT( !samplepos->isValid(V3D(10.0,0.0,0.05)+samplepos->getPos()) );

    // test of source shape
    TS_ASSERT( source->isValid(V3D(0.0,0.0,0.005)+source->getPos()) );
    TS_ASSERT( !source->isValid(V3D(0.0,0.0,-0.005)+source->getPos()) );
    TS_ASSERT( !source->isValid(V3D(0.0,0.0,0.02)+source->getPos()) );

    // Check absence of distinct physical instrument
    TS_ASSERT( !i->getPhysicalInstrument() );
  }


  void test_prase_IDF_for_unit_testing2() // IDF stands for Instrument Definition File
  {
    std::string filename = ConfigService::Instance().getInstrumentDirectory() + "/IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING2.xml";
    std::string xmlText = Strings::loadFile(filename);
    boost::shared_ptr<const Instrument> i;

    // Parse the XML
    InstrumentDefinitionParser parser;
    TS_ASSERT_THROWS_NOTHING( parser.initialize(filename, "For Unit Testing2", xmlText); );
    TS_ASSERT_THROWS_NOTHING( i = parser.parseXML(NULL); );

    boost::shared_ptr<const IDetector> ptrDetShape = i->getDetector(1100);
    TS_ASSERT_EQUALS( ptrDetShape->getID(), 1100);

    // Test of monitor shape
    boost::shared_ptr<const IDetector> ptrMonShape = i->getDetector(1001);
    TS_ASSERT( ptrMonShape->isValid(V3D(0.002,0.0,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.002,0.0,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(0.003,0.0,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(-0.003,0.0,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.0069,0.0227,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(-0.0071,0.0227,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.0069,0.0227,0.009)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(-0.0069,0.0227,0.011)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.1242,0.0,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.0621,0.0621,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.0621,-0.0621,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.0621,0.0641,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(-0.0621,0.0651,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(-0.0621,0.0595,0.0)+ptrMonShape->getPos()) );
    TS_ASSERT( ptrMonShape->isValid(V3D(-0.0621,0.0641,0.01)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(-0.0621,0.0641,0.011)+ptrMonShape->getPos()) );
    TS_ASSERT( !ptrMonShape->isValid(V3D(-0.0621,0.0651,0.01)+ptrMonShape->getPos()) );
  }

  void test_parse_RectangularDetector()
  {
    std::string filename = ConfigService::Instance().getInstrumentDirectory() + "/IDFs_for_UNIT_TESTING/IDF_for_RECTANGULAR_UNIT_TESTING.xml";
    std::string xmlText = Strings::loadFile(filename);
    boost::shared_ptr<const Instrument> i;

    // Parse the XML
    InstrumentDefinitionParser parser;
    TS_ASSERT_THROWS_NOTHING( parser.initialize(filename, "RectangularUnitTest", xmlText); );
    TS_ASSERT_THROWS_NOTHING( i = parser.parseXML(NULL); );

    // Now the XY detector in bank1
    boost::shared_ptr<const RectangularDetector> bank1 = boost::dynamic_pointer_cast<const RectangularDetector>( i->getComponentByName("bank1") );
    TS_ASSERT( bank1 );
    if (!bank1) return;

    //Right # of x columns?
    TS_ASSERT_EQUALS( bank1->nelements(), 100);

    //Positions according to formula
    TS_ASSERT_DELTA( bank1->getAtXY(0,0)->getPos().X(), -0.1, 1e-4 );
    TS_ASSERT_DELTA( bank1->getAtXY(0,0)->getPos().Y(), -0.2, 1e-4 );
    TS_ASSERT_DELTA( bank1->getAtXY(1,0)->getPos().X(), -0.098, 1e-4 );
    TS_ASSERT_DELTA( bank1->getAtXY(1,1)->getPos().Y(), -0.198, 1e-4 );

    //Some IDs
    TS_ASSERT_EQUALS( bank1->getAtXY(0,0)->getID(), 1000);
    TS_ASSERT_EQUALS( bank1->getAtXY(0,1)->getID(), 1001);
    TS_ASSERT_EQUALS( bank1->getAtXY(1,0)->getID(), 1300);
    TS_ASSERT_EQUALS( bank1->getAtXY(1,1)->getID(), 1301);

    //The total number of detectors
    detid2det_map dets;
    i->getDetectors(dets);
    TS_ASSERT_EQUALS( dets.size(), 100*200 * 2);
  }



  void testGetAbsolutPositionInCompCoorSys()
  {
    CompAssembly base("base");
    base.setPos(1.0,1.0,1.0);
    base.rotate(Quat(90.0, V3D(0,0,1)));

    InstrumentDefinitionParser helper;
    V3D test = helper.getAbsolutPositionInCompCoorSys(&base, V3D(1,0,0));

    TS_ASSERT_DELTA( test.X(),  1.0, 0.0001);
    TS_ASSERT_DELTA( test.Y(), 2.0, 0.0001);
    TS_ASSERT_DELTA( test.Z(),  1.0, 0.0001);
  }


  // testing through Loading IDF_for_UNIT_TESTING5.xml method adjust()
  void testAdjust()
  {
    std::string filename = ConfigService::Instance().getInstrumentDirectory() + "/IDFs_for_UNIT_TESTING/IDF_for_UNIT_TESTING5.xml";
    std::string xmlText = Strings::loadFile(filename);
    boost::shared_ptr<const Instrument> i;

    // Parse the XML
    InstrumentDefinitionParser parser;
    TS_ASSERT_THROWS_NOTHING( parser.initialize(filename, "AdjustTest", xmlText); );
    TS_ASSERT_THROWS_NOTHING( i = parser.parseXML(NULL); );

    // None rotated cuboid
    boost::shared_ptr<const IDetector> ptrNoneRot = i->getDetector(1400);
    TS_ASSERT( !ptrNoneRot->isValid(V3D(0.0,0.0,0.0)) );
    TS_ASSERT( ptrNoneRot->isValid(V3D(0.0,0.0,3.0)) );
    TS_ASSERT( !ptrNoneRot->isValid(V3D(0.0,4.5,0.0)) );
    TS_ASSERT( ptrNoneRot->isValid(V3D(0.0,4.5,3.0)) );
    TS_ASSERT( !ptrNoneRot->isValid(V3D(0.0,5.5,3.0)) );
    TS_ASSERT( !ptrNoneRot->isValid(V3D(4.5,0.0,3.0)) );

    // rotated cuboids
    boost::shared_ptr<const IDetector> ptrRot = i->getDetector(1300);
    TS_ASSERT( ptrRot->isValid(V3D(0.0,0.0,0.0)) );
    TS_ASSERT( !ptrRot->isValid(V3D(0.0,0.0,3.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.0,4.5,0.0)) );
    TS_ASSERT( !ptrRot->isValid(V3D(0.0,4.5,3.0)) );
    TS_ASSERT( !ptrRot->isValid(V3D(0.0,7.5,0.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.0,10.0,0.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.0,10.0,4.5)) );
    TS_ASSERT( !ptrRot->isValid(V3D(0.0,10.0,5.5)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.0,10.0,-4.5)) );
    TS_ASSERT( !ptrRot->isValid(V3D(1.5,10.0,0.5)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.5,10.0,0.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(-0.5,10.0,0.0)) );

    // nested rotated cuboids
    ptrRot = i->getDetector(1350);
    TS_ASSERT( ptrRot->isValid(V3D(0.0,0.0,0.0)) );
    TS_ASSERT( !ptrRot->isValid(V3D(0.0,0.0,3.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.0,4.5,0.0)) );
    TS_ASSERT( !ptrRot->isValid(V3D(0.0,4.5,3.0)) );
    TS_ASSERT( !ptrRot->isValid(V3D(0.0,7.5,0.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.0,20.0,0.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.0,20.0,4.5)) );
    TS_ASSERT( !ptrRot->isValid(V3D(0.0,20.0,5.5)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.0,20.0,-4.5)) );
    TS_ASSERT( !ptrRot->isValid(V3D(1.5,20.0,0.5)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.5,20.0,0.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(-0.5,20.0,0.0)) );

    // nested rotated cuboids which shape position moved 
    ptrRot = i->getDetector(1360);
    TS_ASSERT( ptrRot->isValid(V3D(1.0,0.0,0.0)) );
    TS_ASSERT( !ptrRot->isValid(V3D(1.0,0.0,3.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(1.0,4.5,0.0)) );
    TS_ASSERT( !ptrRot->isValid(V3D(1.0,4.5,3.0)) );
    TS_ASSERT( !ptrRot->isValid(V3D(1.0,7.5,0.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(1.0,20.0,0.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(1.0,20.0,4.5)) );
    TS_ASSERT( !ptrRot->isValid(V3D(1.0,20.0,5.5)) );
    TS_ASSERT( ptrRot->isValid(V3D(1.0,20.0,-4.5)) );
    TS_ASSERT( !ptrRot->isValid(V3D(2.5,20.0,0.5)) );
    TS_ASSERT( ptrRot->isValid(V3D(1.5,20.0,0.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.5,20.0,0.0)) );

    // nested rotated cuboids which shape position moved by the
    // opposite amount as the location of its parent 
    ptrRot = i->getDetector(1370);
    TS_ASSERT( ptrRot->isValid(V3D(0.0,0.0,0.0)) );
    TS_ASSERT( !ptrRot->isValid(V3D(0.0,0.0,3.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.0,4.5,0.0)) );
    TS_ASSERT( !ptrRot->isValid(V3D(0.0,4.5,3.0)) );
    TS_ASSERT( !ptrRot->isValid(V3D(0.0,7.5,0.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.0,20.0,0.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.0,20.0,4.5)) );
    TS_ASSERT( !ptrRot->isValid(V3D(0.0,20.0,5.5)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.0,20.0,-4.5)) );
    TS_ASSERT( !ptrRot->isValid(V3D(1.5,20.0,0.5)) );
    TS_ASSERT( ptrRot->isValid(V3D(0.5,20.0,0.0)) );
    TS_ASSERT( ptrRot->isValid(V3D(-0.5,20.0,0.0)) );
  }


};


#endif /* MANTID_GEOMETRY_INSTRUMENTDEFINITIONPARSERTEST_H_ */

