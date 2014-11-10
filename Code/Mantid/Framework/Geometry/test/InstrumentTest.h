#ifndef INSTRUMENTTEST_H_
#define INSTRUMENTTEST_H_

#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Exception.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include <cxxtest/TestSuite.h>
#include "MantidKernel/DateAndTime.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class InstrumentTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentTest *createSuite() { return new InstrumentTest(); }
  static void destroySuite( InstrumentTest *suite ) { delete suite; }

  InstrumentTest()
  {
    instrument.setName("TestInst");
    ObjComponent *source = new ObjComponent("source");
    source->setPos(0.0,0.0,-10.0);
    instrument.add(source);
    instrument.markAsSource(source);
    ObjComponent *sample = new ObjComponent("sample");
    instrument.add(sample);
    instrument.markAsSamplePos(sample);
    det = new Detector("det1",1,0);
    det->setPos(1.0,0.0,0.0);
    instrument.add(det);
    instrument.markAsDetector(det);
    det2 = new Detector("det2",10,0);
    det2->setPos(0.0,1.0,0.0);
    instrument.add(det2);
    instrument.markAsDetector(det2);
    det3 = new Detector("det3",11,0);
    det->setPos(0.0,0.0,1.0);
    instrument.add(det3);
    instrument.markAsMonitor(det3);

    //instrument.setDefaultViewAxis("X-");
    instrument.getLogfileCache().insert(std::make_pair(std::make_pair("apple",det3),boost::shared_ptr<XMLInstrumentParameter>()));
    instrument.getLogfileUnit()["banana"] = "yellow";
  }

  void testType()
  {
    TS_ASSERT_EQUALS( instrument.type(), "Instrument" );
  }

  void testConstructor()
  {
    Instrument i;
    TS_ASSERT( ! i.getSource() );
    TS_ASSERT( ! i.getSample() );
    TS_ASSERT( ! i.isParametrized() );
    TS_ASSERT_THROWS( i.baseInstrument(), std::runtime_error );
    TS_ASSERT_THROWS( i.getParameterMap(), std::runtime_error );

    Instrument ii("anInstrument");
    TS_ASSERT( ! ii.getSource() );
    TS_ASSERT( ! ii.getSample() );
    TS_ASSERT_EQUALS( ii.getName(), "anInstrument" );
  }

  void testCopyConstructor()
  {
    Instrument i = instrument;
    TS_ASSERT_EQUALS( i.getName(), instrument.getName() );
    TS_ASSERT_EQUALS( i.nelements(), instrument.nelements() );
    TS_ASSERT_EQUALS( i.getLogfileCache(), instrument.getLogfileCache() );
    TS_ASSERT_EQUALS( i.getLogfileUnit(), instrument.getLogfileUnit() );
    TS_ASSERT_EQUALS( i.getMonitors(), instrument.getMonitors() );
    TS_ASSERT_EQUALS( i.getDefaultView(), instrument.getDefaultView() );
    TS_ASSERT_EQUALS( i.getDefaultAxis(), instrument.getDefaultAxis() );
    // Should not be parameterized - there's a different constructor for that
    TS_ASSERT_THROWS( i.baseInstrument(), std::runtime_error );
    TS_ASSERT_THROWS( i.getParameterMap(), std::runtime_error );
    TS_ASSERT_EQUALS( i.getValidFromDate(), instrument.getValidFromDate() );
    TS_ASSERT_EQUALS( i.getValidToDate(), instrument.getValidToDate() );

    // Check source and sample copied correctly, but different objects
    TS_ASSERT_EQUALS( i.getSource()->getName(), instrument.getSource()->getName() );
    TS_ASSERT_EQUALS( i.getSource()->getPos(), instrument.getSource()->getPos() );
    TS_ASSERT_DIFFERS( i.getSource(), instrument.getSource() );
    TS_ASSERT_EQUALS( i.getSample()->getName(), instrument.getSample()->getName() );
    TS_ASSERT_EQUALS( i.getSample()->getPos(), instrument.getSample()->getPos() );
    TS_ASSERT_DIFFERS( i.getSample(), instrument.getSample() );

    // Ditto for the detectors
    detid2det_map origMap, copyMap;
    instrument.getDetectors(origMap);
    i.getDetectors(copyMap);
    TS_ASSERT_EQUALS( copyMap.size(), origMap.size() );
    detid2det_map::const_iterator origIt = origMap.begin();
    detid2det_map::const_iterator copyIt = copyMap.begin();
    for ( ; origIt != origMap.end(); ++origIt, ++copyIt )
    {
      TS_ASSERT_EQUALS( copyIt->first, origIt->first );
      TS_ASSERT_DIFFERS( copyIt->second.get(), origIt->second.get() );
      TS_ASSERT_EQUALS( copyIt->second->getName(), origIt->second->getName() );
      TS_ASSERT_EQUALS( copyIt->second->getPos(), origIt->second->getPos() );
    }
  }

  void testClone()
  {
    Instrument* instr = new Instrument("Inst");
    instr->setDefaultViewAxis("Y");
    IComponent* inst = instr;
    IComponent* copy(NULL);
    TS_ASSERT_THROWS_NOTHING( copy = inst->clone() );
    TS_ASSERT_DIFFERS( &*copy, &*inst );
    TS_ASSERT_EQUALS( copy->getName(), inst->getName() );
    Instrument* copyI = dynamic_cast<Instrument*>(copy);
    TS_ASSERT( copyI );
    TS_ASSERT_EQUALS( instr->getDefaultView(), copyI->getDefaultView() );
    TS_ASSERT_EQUALS( instr->getDefaultAxis(), copyI->getDefaultAxis() );
    delete instr;
    delete copy;
  }

  void testSource()
  {
    Instrument i;
    TS_ASSERT( ! i.getSource() );
    ObjComponent *s = new ObjComponent("");
    // Cannot have an unnamed source
    TS_ASSERT_THROWS( i.markAsSource(s), Exception::InstrumentDefinitionError );
    s->setName("source");
    TS_ASSERT_THROWS_NOTHING( i.markAsSource(s) );
    TS_ASSERT_EQUALS( i.getSource().get(), s );
    ObjComponent *ss = new ObjComponent("source2");
    // Trying to add source a second time does nothing
    TS_ASSERT_THROWS_NOTHING( i.markAsSource(ss) );
    TS_ASSERT_EQUALS( i.getSource().get(), s );
    delete s;
    delete ss;
  }

  void test_Marking_Chopper_Point_Without_Defined_Source_Throws_Exception()
  {
    Instrument instr;
    const ObjComponent *chopper = new ObjComponent("chopper1");
    TS_ASSERT_THROWS(instr.markAsChopperPoint(chopper), Mantid::Kernel::Exception::InstrumentDefinitionError);
    delete chopper;
  }

  void test_Marking_Chopper_With_Name_As_Chopper_Succeeds()
  {
    Instrument_sptr instr = createInstrumentWithSource();
    const ObjComponent *chopper = new ObjComponent("chopper1");
    TS_ASSERT_THROWS_NOTHING(instr->markAsChopperPoint(chopper));
    delete chopper; // It was not added to the assembly
  }

  void test_Marking_Unamed_Chopper_As_Chopper_Throws_Exception()
  {
    Instrument_sptr instr = createInstrumentWithSource();
    const ObjComponent *chopper = new ObjComponent("");
    TS_ASSERT_THROWS(instr->markAsChopperPoint(chopper), std::invalid_argument);
    delete chopper; // It was not added to the assembly
  }

  void test_Retrieving_Chopper_With_Invalid_Index_Throws_Exception()
  {
    Instrument_sptr instr = createInstrumentWithSource();
    TS_ASSERT_THROWS(instr->getChopperPoint(0), std::invalid_argument);
  }

  void test_Inserting_Chopper_Closest_To_Source_Gets_Set_To_Index_Zero()
  {
    using Mantid::Kernel::V3D;
    Instrument_sptr instr = createInstrumentWithSource();

    ObjComponent *chopper1 = new ObjComponent("chopper1");
    chopper1->setPos(V3D(0.,0.,-2.0));

    TS_ASSERT_THROWS_NOTHING(instr->markAsChopperPoint(chopper1));
    IObjComponent_const_sptr closestChopper = instr->getChopperPoint(0);
    TS_ASSERT_EQUALS(closestChopper.get(), chopper1);

    ObjComponent *chopper2 = new ObjComponent("chopper2");
    chopper2->setPos(V3D(0.,0.,-9.0)); // source is at -10
    TS_ASSERT_THROWS_NOTHING(instr->markAsChopperPoint(chopper2));
    closestChopper = instr->getChopperPoint(0);
    TS_ASSERT_EQUALS(closestChopper.get(), chopper2);

    IObjComponent_const_sptr otherChopper = instr->getChopperPoint(1);
    TS_ASSERT_EQUALS(otherChopper.get(), chopper1);

    delete chopper1;
    delete chopper2;
  }

  void test_NumberOfChopperPoints_Matches_Number_Marked()
  {
    using Mantid::Kernel::V3D;
    Instrument_sptr instr = createInstrumentWithSource();

    TS_ASSERT_EQUALS(instr->getNumberOfChopperPoints(), 0);

    ObjComponent *chopper1 = new ObjComponent("chopper1");
    instr->markAsChopperPoint(chopper1);
    TS_ASSERT_EQUALS(instr->getNumberOfChopperPoints(), 1);

    ObjComponent *chopper2 = new ObjComponent("chopper2");
    instr->markAsChopperPoint(chopper2);
    TS_ASSERT_EQUALS(instr->getNumberOfChopperPoints(), 2);

    delete chopper1;
    delete chopper2;
  }


  void testSamplePos()
  {
    Instrument i;
    TS_ASSERT( ! i.getSample() );
    ObjComponent *s = new ObjComponent("");
    // Cannot have an unnamed source
    TS_ASSERT_THROWS( i.markAsSamplePos(s), Exception::InstrumentDefinitionError );
    s->setName("sample");
    TS_ASSERT_THROWS_NOTHING( i.markAsSamplePos(s) );
    TS_ASSERT_EQUALS( i.getSample().get(), s );
    ObjComponent *ss = new ObjComponent("sample2");
    // Trying to add sample a second time does nothing
    TS_ASSERT_THROWS_NOTHING( i.markAsSamplePos(ss) );
    TS_ASSERT_EQUALS( i.getSample().get(), s );
    delete s;
    delete ss;
  }

  void testBeamDirection()
  {
    TS_ASSERT_EQUALS( instrument.getBeamDirection(), V3D(0,0,1) );
  }

  void testNumberDetectors()
  { // THIS MUST BE RUN BEFORE testDetector!!!!!!!
    // that test adds a detector to the instrument
    std::size_t ndets(3);
    TS_ASSERT_EQUALS(instrument.getDetectorIDs(false).size(), ndets);
    TS_ASSERT_EQUALS(instrument.getNumberDetectors(false), ndets);
    TS_ASSERT_EQUALS(instrument.getNumberDetectors(true), ndets-1); // skipMonitors
  }

  void testNumMonitors()
  {
    TS_ASSERT_EQUALS( instrument.numMonitors(), 1 );
    TS_ASSERT_EQUALS( Instrument().numMonitors(), 0 );
  }

  void testDetector()
  {
    TS_ASSERT_THROWS( instrument.getDetector(0), Exception::NotFoundError );
    TS_ASSERT_EQUALS( instrument.getDetector(1).get(), det );
    TS_ASSERT_THROWS( instrument.getDetector(2), Exception::NotFoundError );
    Detector *d = new Detector("det",2,0);
    TS_ASSERT_THROWS_NOTHING( instrument.markAsDetector(d) );
    TS_ASSERT_EQUALS( instrument.getDetector(2).get(), d );
    delete d;
  }

  void testRemoveDetector()
  {
    Instrument i;
    Detector *d = new Detector("det",1,&i);
    TS_ASSERT_THROWS_NOTHING( i.markAsDetector(d) );
    TS_ASSERT_EQUALS( i.getDetector(1).get(), d );
    // Next 2 lines demonstrate what can happen if detector cache and CompAssembly tree are inconsistent
    // Unfortunately, the way things were written means that this can happen
    TS_ASSERT_THROWS( i.removeDetector(d), std::runtime_error );
    TS_ASSERT_THROWS( i.getDetector(1).get(), Exception::NotFoundError );
    // Now make the 2 calls necessary to do it properly
    TS_ASSERT_THROWS_NOTHING( i.add(d) );
    TS_ASSERT_THROWS_NOTHING( i.markAsDetector(d) );
    TS_ASSERT_EQUALS( i.getDetectorIDs(false).size(), 1 );
    TS_ASSERT_EQUALS( i.nelements(), 1 );
    TS_ASSERT_THROWS_NOTHING( i.removeDetector(d) );
    TS_ASSERT_THROWS( i.getDetector(1).get(), Exception::NotFoundError );
    TS_ASSERT_EQUALS( i.nelements(), 0 );

    // Now check it does the right thing for a monitor as well
    Detector *m = new Detector("mon",1,&i);
    TS_ASSERT_THROWS_NOTHING( i.add(m) );
    TS_ASSERT_THROWS_NOTHING( i.markAsMonitor(m) );
    TS_ASSERT_EQUALS( i.getMonitors().size(), 1 );
    TS_ASSERT_THROWS_NOTHING( i.removeDetector(m) );
    TS_ASSERT( i.getMonitors().empty() );
    TS_ASSERT( i.getDetectorIDs(false).empty() );
  }

  void test_GetDetectors_With_All_Valid_IDs()
  {
    const size_t ndets(3);
    std::vector<detid_t> detIDs(ndets);
    detIDs[0] = 1;
    detIDs[1] = 10;
    detIDs[2] = 11;

    std::vector<IDetector_const_sptr> dets;
    TS_ASSERT_THROWS_NOTHING(dets = instrument.getDetectors(detIDs));
    TS_ASSERT_EQUALS(dets.size(), ndets);
    for( size_t i = 0; i < ndets; ++i )
    {
      TS_ASSERT_EQUALS(dets[i]->getID(), detIDs[i]);
    }
  }

  void test_GetDetector_With_A_List_Returns_A_Group()
  {
    const size_t ndets(3);
    std::vector<detid_t> detIDs(ndets);
    detIDs[0] = 1;
    detIDs[1] = 10;
    detIDs[2] = 11;

    IDetector_const_sptr det;
    TS_ASSERT_THROWS_NOTHING(det = instrument.getDetectorG(detIDs));
    boost::shared_ptr<const DetectorGroup> detGroup = boost::dynamic_pointer_cast<const DetectorGroup>(det);
    TS_ASSERT(detGroup);
    
    TS_ASSERT_EQUALS(detGroup->nDets(), ndets);
    std::vector<detid_t> memberIDs = detGroup->getDetectorIDs();
    for( size_t i = 0; i < ndets; ++i )
    {
      TS_ASSERT_EQUALS(memberIDs[i], detIDs[i]);
    }
  }

  void test_GetDetectors_Throws_With_Invalid_IDs()
  {
    const size_t ndets(1);
    std::vector<detid_t> detIDs(ndets);
    detIDs[0] = 10000;

    std::vector<IDetector_const_sptr> dets;
    TS_ASSERT_THROWS(dets = instrument.getDetectors(detIDs), Kernel::Exception::NotFoundError);
    
  }


  void testCasts()
  {
    Instrument *i = new Instrument;
    TS_ASSERT( dynamic_cast<CompAssembly*>(i) );
    TS_ASSERT( dynamic_cast<Component*>(i) );
    delete i;
  }

  void testIDs()
  {
      ComponentID  id1 = det->getComponentID();
      TS_ASSERT_EQUALS(det->getName(), instrument.getComponentByID(id1)->getName() );

      ComponentID  id2 = det2->getComponentID();
      TS_ASSERT_EQUALS(det2->getName(), instrument.getComponentByID(id2)->getName() );

      ComponentID  id3 = det3->getComponentID();
      TS_ASSERT_EQUALS(det3->getName(), instrument.getComponentByID(id3)->getName() );

  }

  void testGetByName()
  {
    Instrument *i = new Instrument;
    i->setName("TestInstrument");
    
    CompAssembly *bank = new CompAssembly("bank");
    bank->setPos(1.,0,1.);
    Quat q(0.9,0,0,0.2);
    q.normalize();
    bank->setRot(q);
    i->add(bank);

    Detector *det = new Detector("det1",1,0);
    det->setPos(1.0,0.0,0.0);
    bank->add(det);
    i->markAsDetector(det);

    // Instrument name
    TS_ASSERT( i->getComponentByName("TestInstrument").get() );
    // Bank
    TS_ASSERT( i->getComponentByName("bank").get() );
    //Det 1
    TS_ASSERT( i->getComponentByName("det1").get() );
    // Whole path
    TS_ASSERT( i->getComponentByName("TestInstrument/bank/det1").get() );
    // Path with 'bank' skipped
    TS_ASSERT( i->getComponentByName("TestInstrument/det1").get() );
    // Path starting from 'bank'
    TS_ASSERT( i->getComponentByName("bank/det1").get() );

    delete i;
  }



  void test_getDetectorsInBank()
  {
    // 5 banks with 6x6 pixels in them.
    Instrument_const_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(5, 6);
    std::vector<IDetector_const_sptr> dets;
    inst->getDetectorsInBank(dets, "bank2");
    TS_ASSERT_EQUALS(dets.size(), 36);
    TS_ASSERT_EQUALS(dets[0]->getID(), 36*2);
  }

  void test_getDetectorsInBank2()
  {
    // 5 banks with 9 pixels each
    Instrument_const_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5, false);
    std::vector<IDetector_const_sptr> dets;
    inst->getDetectorsInBank(dets, "bank2");
    TS_ASSERT_EQUALS(dets.size(), 9);
  }

  void test_getDetectors()
  {
    // 5 banks with 6x6 pixels in them.
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(5, 6);
    detid2det_map dets;
    inst->getDetectors(dets);
    TS_ASSERT_EQUALS(dets.size(), 36*5);
  }

  void test_getDetectorIDs()
  {
    // 5 banks with 6x6 pixels in them.
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(5, 6);
    std::vector<detid_t> dets;
    dets = inst->getDetectorIDs();
    TS_ASSERT_EQUALS(dets.size(), 36*5);

    TS_ASSERT_EQUALS(inst->getNumberDetectors(false), 5*6*6);
    TS_ASSERT_EQUALS(inst->getNumberDetectors(true), 5*6*6); // skipMonitors
  }

  void test_getValidFromDate()
  {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(5, 6);
    Kernel::DateAndTime validFrom("1900-01-31T23:59:59");
    Kernel::DateAndTime validTo("2100-01-31 23:59:59");
    inst->setValidFromDate(validFrom);
    inst->setValidToDate(validTo);
    TS_ASSERT_EQUALS( inst->getValidFromDate(), validFrom);
    TS_ASSERT_EQUALS( inst->getValidToDate(), validTo);
    // Try the parametrized copy constructor
    ParameterMap_sptr map(new ParameterMap());
    Instrument inst2(boost::dynamic_pointer_cast<Instrument>(inst), map);
    TS_ASSERT_EQUALS( inst2.getValidFromDate(), validFrom);
    TS_ASSERT_EQUALS( inst2.getValidToDate(), validTo);
  }

  void test_getMinMaxDetectorIDs()
  {
    detid_t min = 0;
    detid_t max = 0;
    TS_ASSERT_THROWS_NOTHING( instrument.getMinMaxDetectorIDs(min,max) );
    TS_ASSERT_EQUALS( min, 1);
    TS_ASSERT_EQUALS( max, 11);
  }

  void test_default_view()
  {
    Instrument i;
    TS_ASSERT_EQUALS( i.getDefaultView(), "3D" );
    TS_ASSERT_EQUALS( i.getDefaultAxis(), "Z+" );

    i.setDefaultView( "CYLINDRICAL_Y" );
    TS_ASSERT_EQUALS( i.getDefaultView(), "CYLINDRICAL_Y" );
    i.setDefaultView( "spherical_y" );
    TS_ASSERT_EQUALS( i.getDefaultView(), "SPHERICAL_Y" );
    i.setDefaultView( "inside-out" );
    TS_ASSERT_EQUALS( i.getDefaultView(), "3D" );
  }

  void testContainsRectDetectors()
  {
    Instrument_sptr instrFull 
      = ComponentCreationHelper::createTestInstrumentRectangular(5, 3);

    TS_ASSERT_EQUALS(instrFull->containsRectDetectors(), 
                     Instrument::ContainsState::Full);

    Instrument_sptr instrPartial
      = ComponentCreationHelper::createTestInstrumentRectangular(5, 3);

    // Add some non-rectangular component
    instrPartial->add(new Component("Component"));

    TS_ASSERT_EQUALS(instrPartial->containsRectDetectors(), 
                     Instrument::ContainsState::Partial);


    Instrument_sptr instrNone 
      = ComponentCreationHelper::createTestInstrumentCylindrical(5, false);

    TS_ASSERT_EQUALS(instrNone->containsRectDetectors(),
                     Instrument::ContainsState::None);
  }

  void testContainsRectDetectorsRecursive()
  {
    Instrument_sptr instrRect
      = ComponentCreationHelper::createTestInstrumentRectangular(5, 3);

    CompAssembly* newAssembly1 = new CompAssembly("Assembly 1");
    CompAssembly* newAssembly2 = new CompAssembly("Assembly 2");

    RectangularDetector* rectDet1 = new RectangularDetector("Rect Detector 1");
    RectangularDetector* rectDet2 = new RectangularDetector("Rect Detector 2");

    newAssembly2->add(rectDet2);

    newAssembly1->add(rectDet1);
    newAssembly1->add(newAssembly2);

    instrRect->add(newAssembly1);

    TS_ASSERT_EQUALS(instrRect->containsRectDetectors(),
                     Instrument::ContainsState::Full);

    instrRect->add(new Component("Component"));

    TS_ASSERT_EQUALS(instrRect->containsRectDetectors(),
                     Instrument::ContainsState::Partial);
  }

private:

  Instrument_sptr createInstrumentWithSource()
  {
    using Mantid::Kernel::V3D;
    Instrument_sptr instr(new Instrument);
    ObjComponent *source = new ObjComponent("source");
    source->setPos(V3D(0.,0.,-10.));
    instr->add(source);
    instr->markAsSource(source);

    return instr;
  }


  Instrument instrument;
  Detector *det, *det2, *det3;
};

#endif /*INSTRUMENTTEST_H_*/
