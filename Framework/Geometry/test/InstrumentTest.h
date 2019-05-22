// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef INSTRUMENTTEST_H_
#define INSTRUMENTTEST_H_

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/Exception.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class InstrumentTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentTest *createSuite() { return new InstrumentTest(); }
  static void destroySuite(InstrumentTest *suite) { delete suite; }

  InstrumentTest() {
    instrument.setName("TestInst");
    ObjComponent *source = new ObjComponent("source");
    source->setPos(0.0, 0.0, -10.0);
    instrument.add(source);
    instrument.markAsSource(source);
    ObjComponent *sample = new ObjComponent("sample");
    instrument.add(sample);
    instrument.markAsSamplePos(sample);
    det = new Detector("det1", 1, nullptr);
    det->setPos(1.0, 0.0, 0.0);
    instrument.add(det);
    instrument.markAsDetector(det);
    det2 = new Detector("det2", 10, nullptr);
    det2->setPos(0.0, 1.0, 0.0);
    instrument.add(det2);
    instrument.markAsDetector(det2);
    det3 = new Detector("det3", 11, nullptr);
    det->setPos(0.0, 0.0, 1.0);
    instrument.add(det3);
    instrument.markAsMonitor(det3);

    // instrument.setDefaultViewAxis("X-");
    instrument.getLogfileCache().insert(
        std::make_pair(std::make_pair("apple", det3),
                       boost::shared_ptr<XMLInstrumentParameter>()));
    instrument.getLogfileUnit()["banana"] = "yellow";
  }

  void testType() { TS_ASSERT_EQUALS(instrument.type(), "Instrument"); }

  void testConstructor() {
    Instrument i;
    TS_ASSERT(!i.getSource());
    TS_ASSERT(!i.getSample());
    TS_ASSERT(!i.isParametrized());
    TS_ASSERT_THROWS(i.baseInstrument(), const std::runtime_error &);
    TS_ASSERT_THROWS(i.getParameterMap(), const std::runtime_error &);

    Instrument ii("anInstrument");
    TS_ASSERT(!ii.getSource());
    TS_ASSERT(!ii.getSample());
    TS_ASSERT_EQUALS(ii.getName(), "anInstrument");
  }

  void testCopyConstructor() {
    Instrument i = instrument;
    TS_ASSERT_EQUALS(i.getName(), instrument.getName());
    TS_ASSERT_EQUALS(i.nelements(), instrument.nelements());
    TS_ASSERT_EQUALS(i.getLogfileCache(), instrument.getLogfileCache());
    TS_ASSERT_EQUALS(i.getLogfileUnit(), instrument.getLogfileUnit());
    TS_ASSERT_EQUALS(i.getMonitors(), instrument.getMonitors());
    TS_ASSERT_EQUALS(i.getDefaultView(), instrument.getDefaultView());
    TS_ASSERT_EQUALS(i.getDefaultAxis(), instrument.getDefaultAxis());
    // Should not be parameterized - there's a different constructor for that
    TS_ASSERT_THROWS(i.baseInstrument(), const std::runtime_error &);
    TS_ASSERT_THROWS(i.getParameterMap(), const std::runtime_error &);
    TS_ASSERT_EQUALS(i.getValidFromDate(), instrument.getValidFromDate());
    TS_ASSERT_EQUALS(i.getValidToDate(), instrument.getValidToDate());

    // Check source and sample copied correctly, but different objects
    TS_ASSERT_EQUALS(i.getSource()->getName(),
                     instrument.getSource()->getName());
    TS_ASSERT_EQUALS(i.getSource()->getPos(), instrument.getSource()->getPos());
    TS_ASSERT_DIFFERS(i.getSource(), instrument.getSource());
    TS_ASSERT_EQUALS(i.getSample()->getName(),
                     instrument.getSample()->getName());
    TS_ASSERT_EQUALS(i.getSample()->getPos(), instrument.getSample()->getPos());
    TS_ASSERT_DIFFERS(i.getSample(), instrument.getSample());

    // Ditto for the detectors
    detid2det_map origMap, copyMap;
    instrument.getDetectors(origMap);
    i.getDetectors(copyMap);
    TS_ASSERT_EQUALS(copyMap.size(), origMap.size());
    detid2det_map::const_iterator origIt = origMap.begin();
    detid2det_map::const_iterator copyIt = copyMap.begin();
    for (; origIt != origMap.end(); ++origIt, ++copyIt) {
      TS_ASSERT_EQUALS(copyIt->first, origIt->first);
      TS_ASSERT_DIFFERS(copyIt->second.get(), origIt->second.get());
      TS_ASSERT_EQUALS(copyIt->second->getName(), origIt->second->getName());
      TS_ASSERT_EQUALS(copyIt->second->getPos(), origIt->second->getPos());
    }
  }

  void testClone() {
    Instrument *instr = new Instrument("Inst");
    instr->setDefaultViewAxis("Y");
    IComponent *inst = instr;
    IComponent *copy(nullptr);
    TS_ASSERT_THROWS_NOTHING(copy = inst->clone());
    TS_ASSERT_DIFFERS(&*copy, &*inst);
    TS_ASSERT_EQUALS(copy->getName(), inst->getName());
    Instrument *copyI = dynamic_cast<Instrument *>(copy);
    TS_ASSERT(copyI);
    TS_ASSERT_EQUALS(instr->getDefaultView(), copyI->getDefaultView());
    TS_ASSERT_EQUALS(instr->getDefaultAxis(), copyI->getDefaultAxis());
    delete instr;
    delete copy;
  }

  void testSource() {
    Instrument i;
    TS_ASSERT(!i.getSource());
    ObjComponent *s = new ObjComponent("");
    // Cannot have an unnamed source
    TS_ASSERT_THROWS(i.markAsSource(s), const Exception::InstrumentDefinitionError &);
    s->setName("source");
    TS_ASSERT_THROWS_NOTHING(i.markAsSource(s));
    TS_ASSERT_EQUALS(i.getSource().get(), s);
    ObjComponent *ss = new ObjComponent("source2");
    // Trying to add source a second time does nothing
    TS_ASSERT_THROWS_NOTHING(i.markAsSource(ss));
    TS_ASSERT_EQUALS(i.getSource().get(), s);
    delete s;
    delete ss;
  }

  void test_Marking_Chopper_Point_Without_Defined_Source_Throws_Exception() {
    Instrument instr;
    const ObjComponent *chopper = new ObjComponent("chopper1");
    TS_ASSERT_THROWS(
        instr.markAsChopperPoint(chopper),
        const Mantid::Kernel::Exception::InstrumentDefinitionError &);
    delete chopper;
  }

  void test_Marking_Chopper_With_Name_As_Chopper_Succeeds() {
    Instrument_sptr instr = createInstrumentWithSource();
    const ObjComponent *chopper = new ObjComponent("chopper1");
    TS_ASSERT_THROWS_NOTHING(instr->markAsChopperPoint(chopper));
    delete chopper; // It was not added to the assembly
  }

  void test_Marking_Unamed_Chopper_As_Chopper_Throws_Exception() {
    Instrument_sptr instr = createInstrumentWithSource();
    const ObjComponent *chopper = new ObjComponent("");
    TS_ASSERT_THROWS(instr->markAsChopperPoint(chopper),
                     const std::invalid_argument &);
    delete chopper; // It was not added to the assembly
  }

  void test_Retrieving_Chopper_With_Invalid_Index_Throws_Exception() {
    Instrument_sptr instr = createInstrumentWithSource();
    TS_ASSERT_THROWS(instr->getChopperPoint(0), const std::invalid_argument &);
  }

  void test_Inserting_Chopper_Closest_To_Source_Gets_Set_To_Index_Zero() {
    using Mantid::Kernel::V3D;
    Instrument_sptr instr = createInstrumentWithSource();

    ObjComponent *chopper1 = new ObjComponent("chopper1");
    chopper1->setPos(V3D(0., 0., -2.0));

    TS_ASSERT_THROWS_NOTHING(instr->markAsChopperPoint(chopper1));
    IObjComponent_const_sptr closestChopper = instr->getChopperPoint(0);
    TS_ASSERT_EQUALS(closestChopper.get(), chopper1);

    ObjComponent *chopper2 = new ObjComponent("chopper2");
    chopper2->setPos(V3D(0., 0., -9.0)); // source is at -10
    TS_ASSERT_THROWS_NOTHING(instr->markAsChopperPoint(chopper2));
    closestChopper = instr->getChopperPoint(0);
    TS_ASSERT_EQUALS(closestChopper.get(), chopper2);

    IObjComponent_const_sptr otherChopper = instr->getChopperPoint(1);
    TS_ASSERT_EQUALS(otherChopper.get(), chopper1);

    delete chopper1;
    delete chopper2;
  }

  void test_NumberOfChopperPoints_Matches_Number_Marked() {
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

  void testSamplePos() {
    Instrument i;
    TS_ASSERT(!i.getSample());
    ObjComponent *s = new ObjComponent("");
    // Cannot have an unnamed source
    TS_ASSERT_THROWS(i.markAsSamplePos(s),
                     const Exception::InstrumentDefinitionError &);
    s->setName("sample");
    TS_ASSERT_THROWS_NOTHING(i.markAsSamplePos(s));
    TS_ASSERT_EQUALS(i.getSample().get(), s);
    ObjComponent *ss = new ObjComponent("sample2");
    // Trying to add sample a second time does nothing
    TS_ASSERT_THROWS_NOTHING(i.markAsSamplePos(ss));
    TS_ASSERT_EQUALS(i.getSample().get(), s);
    delete s;
    delete ss;
  }

  void testBeamDirection() {
    TS_ASSERT_EQUALS(instrument.getBeamDirection(), V3D(0, 0, 1));
  }

  void testNumberDetectors() { // THIS MUST BE RUN BEFORE testDetector!!!!!!!
    // that test adds a detector to the instrument
    std::size_t ndets(3);
    TS_ASSERT_EQUALS(instrument.getDetectorIDs(false).size(), ndets);
    TS_ASSERT_EQUALS(instrument.getNumberDetectors(false), ndets);
    TS_ASSERT_EQUALS(instrument.getNumberDetectors(true),
                     ndets - 1); // skipMonitors
  }

  void testDetector() {
    TS_ASSERT_THROWS(instrument.getDetector(0),
                     const Exception::NotFoundError &);
    TS_ASSERT_EQUALS(instrument.getDetector(1).get(), det);
    TS_ASSERT_THROWS(instrument.getDetector(2),
                     const Exception::NotFoundError &);
    Detector *d = new Detector("det", 2, nullptr);
    TS_ASSERT_THROWS_NOTHING(instrument.markAsDetector(d));
    TS_ASSERT_EQUALS(instrument.getDetector(2).get(), d);
    delete d;
  }

  void testRemoveDetector() {
    Instrument i;
    Detector *d = new Detector("det", 1, &i);
    TS_ASSERT_THROWS_NOTHING(i.markAsDetector(d));
    TS_ASSERT_EQUALS(i.getDetector(1).get(), d);
    // Next 2 lines demonstrate what can happen if detector cache and
    // CompAssembly tree are inconsistent
    // Unfortunately, the way things were written means that this can happen
    TS_ASSERT_THROWS(i.removeDetector(d), const std::runtime_error &);
    TS_ASSERT_THROWS(i.getDetector(1).get(), const Exception::NotFoundError &);
    // Now make the 2 calls necessary to do it properly
    TS_ASSERT_THROWS_NOTHING(i.add(d));
    TS_ASSERT_THROWS_NOTHING(i.markAsDetector(d));
    TS_ASSERT_EQUALS(i.getDetectorIDs(false).size(), 1);
    TS_ASSERT_EQUALS(i.nelements(), 1);
    TS_ASSERT_THROWS_NOTHING(i.removeDetector(d));
    TS_ASSERT_THROWS(i.getDetector(1).get(), const Exception::NotFoundError &);
    TS_ASSERT_EQUALS(i.nelements(), 0);

    // Now check it does the right thing for a monitor as well
    Detector *m = new Detector("mon", 1, &i);
    TS_ASSERT_THROWS_NOTHING(i.add(m));
    TS_ASSERT_THROWS_NOTHING(i.markAsMonitor(m));
    TS_ASSERT_EQUALS(i.getMonitors().size(), 1);
    TS_ASSERT_THROWS_NOTHING(i.removeDetector(m));
    TS_ASSERT(i.getMonitors().empty());
    TS_ASSERT(i.getDetectorIDs(false).empty());
  }

  void test_GetDetectors_With_All_Valid_IDs() {
    const size_t ndets(3);
    std::vector<detid_t> detIDs(ndets);
    detIDs[0] = 1;
    detIDs[1] = 10;
    detIDs[2] = 11;

    std::vector<IDetector_const_sptr> dets;
    TS_ASSERT_THROWS_NOTHING(dets = instrument.getDetectors(detIDs));
    TS_ASSERT_EQUALS(dets.size(), ndets);
    for (size_t i = 0; i < ndets; ++i) {
      TS_ASSERT_EQUALS(dets[i]->getID(), detIDs[i]);
    }
  }

  void test_GetDetector_With_A_List_Returns_A_Group() {
    const size_t ndets(3);
    std::set<detid_t> detIDs{1, 10, 11};
    std::vector<detid_t> detIDsVec(detIDs.begin(), detIDs.end());

    IDetector_const_sptr det;
    TS_ASSERT_THROWS_NOTHING(det = instrument.getDetectorG(detIDs));
    boost::shared_ptr<const DetectorGroup> detGroup =
        boost::dynamic_pointer_cast<const DetectorGroup>(det);
    TS_ASSERT(detGroup);

    TS_ASSERT_EQUALS(detGroup->nDets(), ndets);
    std::vector<detid_t> memberIDs = detGroup->getDetectorIDs();
    for (size_t i = 0; i < ndets; ++i) {
      TS_ASSERT_EQUALS(memberIDs[i], detIDsVec[i]);
    }
  }

  void test_GetDetectors_Throws_With_Invalid_IDs() {
    std::set<detid_t> detIDs{10000};

    std::vector<IDetector_const_sptr> dets;
    TS_ASSERT_THROWS(dets = instrument.getDetectors(detIDs),
                     const Kernel::Exception::NotFoundError &);
  }

  void testCasts() {
    Instrument *i = new Instrument;
    TS_ASSERT(dynamic_cast<CompAssembly *>(i));
    TS_ASSERT(dynamic_cast<Component *>(i));
    delete i;
  }

  void testIDs() {
    ComponentID id1 = det->getComponentID();
    TS_ASSERT_EQUALS(det->getName(),
                     instrument.getComponentByID(id1)->getName());

    ComponentID id2 = det2->getComponentID();
    TS_ASSERT_EQUALS(det2->getName(),
                     instrument.getComponentByID(id2)->getName());

    ComponentID id3 = det3->getComponentID();
    TS_ASSERT_EQUALS(det3->getName(),
                     instrument.getComponentByID(id3)->getName());
  }

  void testGetByName() {
    Instrument *i = new Instrument;
    i->setName("TestInstrument");

    CompAssembly *bank = new CompAssembly("bank");
    bank->setPos(1., 0, 1.);
    Quat q(0.9, 0, 0, 0.2);
    q.normalize();
    bank->setRot(q);
    i->add(bank);

    Detector *det = new Detector("det1", 1, nullptr);
    det->setPos(1.0, 0.0, 0.0);
    bank->add(det);
    i->markAsDetector(det);

    // Instrument name
    TS_ASSERT(i->getComponentByName("TestInstrument").get());
    // Bank
    TS_ASSERT(i->getComponentByName("bank").get());
    // Det 1
    TS_ASSERT(i->getComponentByName("det1").get());
    // Whole path
    TS_ASSERT(i->getComponentByName("TestInstrument/bank/det1").get());
    // Path with 'bank' skipped
    TS_ASSERT(i->getComponentByName("TestInstrument/det1").get());
    // Path starting from 'bank'
    TS_ASSERT(i->getComponentByName("bank/det1").get());

    delete i;
  }

  void test_getDetectorsInBank() {
    // 5 banks with 6x6 pixels in them.
    Instrument_const_sptr inst =
        ComponentCreationHelper::createTestInstrumentRectangular(5, 6);
    std::vector<IDetector_const_sptr> dets;
    inst->getDetectorsInBank(dets, "bank2");
    TS_ASSERT_EQUALS(dets.size(), 36);
    TS_ASSERT_EQUALS(dets[0]->getID(), 36 * 2);
  }

  void test_getDetectorsInBank2() {
    // 5 banks with 9 pixels each
    Instrument_const_sptr inst =
        ComponentCreationHelper::createTestInstrumentCylindrical(5);
    std::vector<IDetector_const_sptr> dets;
    inst->getDetectorsInBank(dets, "bank2");
    TS_ASSERT_EQUALS(dets.size(), 9);
  }

  void test_getDetectorsInBank_throwsIfBankNotFound() {
    Instrument_const_sptr inst =
        ComponentCreationHelper::createTestInstrumentRectangular(5, 6);
    std::vector<IDetector_const_sptr> dets;
    TS_ASSERT_THROWS(inst->getDetectorsInBank(dets, "bank_in_the_dark_side"),
                     const Exception::NotFoundError &)
  }

  void test_getDetectors() {
    // 5 banks with 6x6 pixels in them.
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentRectangular(5, 6);
    detid2det_map dets;
    inst->getDetectors(dets);
    TS_ASSERT_EQUALS(dets.size(), 36 * 5);
  }

  void test_getDetectorIDs() {
    // 5 banks with 6x6 pixels in them.
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentRectangular(5, 6);
    std::vector<detid_t> dets;
    dets = inst->getDetectorIDs();
    TS_ASSERT_EQUALS(dets.size(), 36 * 5);

    TS_ASSERT_EQUALS(inst->getNumberDetectors(false), 5 * 6 * 6);
    TS_ASSERT_EQUALS(inst->getNumberDetectors(true), 5 * 6 * 6); // skipMonitors
  }

  void test_getValidFromDate() {
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentRectangular(5, 6);
    Types::Core::DateAndTime validFrom("1900-01-31T23:59:59");
    Types::Core::DateAndTime validTo("2100-01-31 23:59:59");
    inst->setValidFromDate(validFrom);
    inst->setValidToDate(validTo);
    TS_ASSERT_EQUALS(inst->getValidFromDate(), validFrom);
    TS_ASSERT_EQUALS(inst->getValidToDate(), validTo);
    // Try the parametrized copy constructor
    ParameterMap_sptr map(new ParameterMap());
    Instrument inst2(boost::dynamic_pointer_cast<Instrument>(inst), map);
    TS_ASSERT_EQUALS(inst2.getValidFromDate(), validFrom);
    TS_ASSERT_EQUALS(inst2.getValidToDate(), validTo);
  }

  void test_getMinMaxDetectorIDs() {
    detid_t min = 0;
    detid_t max = 0;
    TS_ASSERT_THROWS_NOTHING(instrument.getMinMaxDetectorIDs(min, max));
    TS_ASSERT_EQUALS(min, 1);
    TS_ASSERT_EQUALS(max, 11);
  }

  void test_default_view() {
    Instrument i;
    TS_ASSERT_EQUALS(i.getDefaultView(), "3D");
    TS_ASSERT_EQUALS(i.getDefaultAxis(), "Z+");

    i.setDefaultView("CYLINDRICAL_Y");
    TS_ASSERT_EQUALS(i.getDefaultView(), "CYLINDRICAL_Y");
    i.setDefaultView("spherical_y");
    TS_ASSERT_EQUALS(i.getDefaultView(), "SPHERICAL_Y");
    i.setDefaultView("inside-out");
    TS_ASSERT_EQUALS(i.getDefaultView(), "3D");
  }

  void testContainsRectDetectors() {
    Instrument_sptr instrFull =
        ComponentCreationHelper::createTestInstrumentRectangular(5, 3);

    TS_ASSERT_EQUALS(instrFull->containsRectDetectors(),
                     Instrument::ContainsState::Full);

    Instrument_sptr instrPartial =
        ComponentCreationHelper::createTestInstrumentRectangular(5, 3);

    // Add some non-rectangular component
    instrPartial->add(new Component("Component"));

    TS_ASSERT_EQUALS(instrPartial->containsRectDetectors(),
                     Instrument::ContainsState::Partial);

    Instrument_sptr instrNone =
        ComponentCreationHelper::createTestInstrumentCylindrical(5);

    TS_ASSERT_EQUALS(instrNone->containsRectDetectors(),
                     Instrument::ContainsState::None);
  }

  void testContainsRectDetectorsRecursive() {
    Instrument_sptr instrRect =
        ComponentCreationHelper::createTestInstrumentRectangular(5, 3);

    CompAssembly *newAssembly1 = new CompAssembly("Assembly 1");
    CompAssembly *newAssembly2 = new CompAssembly("Assembly 2");

    RectangularDetector *rectDet1 = new RectangularDetector("Rect Detector 1");
    RectangularDetector *rectDet2 = new RectangularDetector("Rect Detector 2");

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

  void test_detectorIndex() {
    auto i = ComponentCreationHelper::createTestInstrumentRectangular(1, 2);
    TS_ASSERT_EQUALS(i->detectorIndex(4), 0);
    TS_ASSERT_EQUALS(i->detectorIndex(5), 1);
    TS_ASSERT_EQUALS(i->detectorIndex(6), 2);
    TS_ASSERT_EQUALS(i->detectorIndex(7), 3);
  }

  void test_makeLegacyParameterMap() {
    const auto &baseInstrument =
        ComponentCreationHelper::createTestInstrumentCylindrical(3);
    const auto bank1 = baseInstrument->getComponentByName("bank1");
    const auto bank2 = baseInstrument->getComponentByName("bank2");
    const auto bank3 = baseInstrument->getComponentByName("bank3");
    const V3D bankOffset{0.1, 0.2, 0.3};
    const V3D bankEpsilon{5e-10, 5e-10, 5e-10};
    const V3D bankAxis{0, 0, 1};
    const Quat bankRot(90.0, bankAxis);
    const V3D bankScale{1, 2, 3};
    auto pmap = boost::make_shared<ParameterMap>();
    pmap->addV3D(bank1->getComponentID(), ParameterMap::pos(), bankOffset);
    pmap->addV3D(bank2->getComponentID(), ParameterMap::pos(), bankEpsilon);
    pmap->addQuat(bank3->getComponentID(), ParameterMap::rot(), bankRot);
    pmap->addV3D(bank3->getComponentID(), ParameterMap::scale(), bankScale);

    // Set instrument in ParameterMap to create DetectorInfo
    pmap->setInstrument(baseInstrument.get());
    auto instr = boost::make_shared<Instrument>(baseInstrument, pmap);
    auto &detInfo = pmap->mutableDetectorInfo();
    auto &compInfo = pmap->mutableComponentInfo();

    // bank 1
    TS_ASSERT(toVector3d(detInfo.position(0))
                  .isApprox(toVector3d(bankOffset + V3D{-0.008, -0.0002, 0.0}),
                            1e-12));
    TS_ASSERT(toVector3d(detInfo.position(2))
                  .isApprox(toVector3d(bankOffset + V3D{0.008, -0.0002, 0.0}),
                            1e-12));
    // bank 2
    TS_ASSERT(toVector3d(detInfo.position(9))
                  .isApprox(toVector3d(bankEpsilon + V3D{-0.008, -0.0002, 0.0}),
                            1e-12));
    // bank 3
    TS_ASSERT(toVector3d(detInfo.position(18))
                  .isApprox(Eigen::Vector3d(0.0002, -0.008, 15.0), 1e-12));
    TS_ASSERT_EQUALS(
        compInfo.scaleFactor(compInfo.indexOf(bank3->getComponentID())),
        bankScale);

    const V3D detOffset{0.2, 0.3, 0.4};
    const V3D detEpsilon{5e-10, 5e-10, 5e-10};
    const V3D detAxis{0.2, 0.4, 13.3};
    const Quat detRot(42.0, detAxis);
    const Quat detRotEps(1e-11, detAxis);

    detInfo.setPosition(0, detInfo.position(0) + detOffset);
    detInfo.setRotation(18, detRot * detInfo.rotation(18));
    // Shifts/rotations by epsilon below tolerance, should not generate
    // parameter from this:
    detInfo.setPosition(1, detInfo.position(1) + detEpsilon);
    detInfo.setPosition(9, detInfo.position(9) + detEpsilon);
    detInfo.setRotation(19, detRotEps * detInfo.rotation(19));
    // Set a new scale factor
    const V3D newScaleFactor{2, 2, 2};
    compInfo.setScaleFactor(compInfo.indexOf(bank3->getComponentID()),
                            newScaleFactor);

    // All position information should be purged
    TS_ASSERT_EQUALS(pmap->size(), 0);

    const auto legacyMap = instr->makeLegacyParameterMap();
    // 3 bank parameters + 2 det parameters + 1 scale parameter
    TS_ASSERT_EQUALS(legacyMap->size(), 6);
    TS_ASSERT(!legacyMap->hasDetectorInfo(baseInstrument.get()));
    Instrument legacyInstrument(baseInstrument, legacyMap);

    TS_ASSERT_EQUALS(legacyInstrument.getDetector(1)->getPos(),
                     bankOffset + V3D(-0.008, -0.0002, 0.0) + detOffset);
    // Was shifted by something less than epsilon so this is the default
    // position.
    TS_ASSERT_EQUALS(legacyInstrument.getDetector(3)->getPos(),
                     bankOffset + V3D(0.008, -0.0002, 0.0));
    // Epsilon in parent is preserved, but not epsilon relative to parent.
    TS_ASSERT_EQUALS(legacyInstrument.getDetector(10)->getPos(),
                     bankEpsilon + V3D(-0.008, -0.0002, 0.0));
    TS_ASSERT_EQUALS(legacyInstrument.getDetector(19)->getPos(),
                     V3D(0.0002, -0.008, 15.0));
    TS_ASSERT(toQuaterniond(legacyInstrument.getDetector(19)->getRotation())
                  .isApprox(toQuaterniond(detRot * bankRot), 1e-10));
    // Check the scale factor
    TS_ASSERT(
        toVector3d(
            legacyInstrument.getComponentByName("bank3")->getScaleFactor())
            .isApprox(toVector3d(newScaleFactor), 1e-10));
  }

  void test_makeLegacyParameterMap_scaled_RectangularDetector() {
    const auto &baseInstrument =
        ComponentCreationHelper::createTestInstrumentRectangular(1, 2);
    const auto bank1 = baseInstrument->getComponentByName("bank1");
    auto pmap = boost::make_shared<ParameterMap>();
    double scalex = 1.7;
    double scaley = 1.3;
    pmap->addDouble(bank1->getComponentID(), "scalex", scalex);
    pmap->addDouble(bank1->getComponentID(), "scaley", scaley);

    // Set instrument in ParameterMap to create DetectorInfo
    pmap->setInstrument(baseInstrument.get());
    auto instr = boost::make_shared<Instrument>(baseInstrument, pmap);
    auto &detInfo = pmap->mutableDetectorInfo();

    // bank 1
    double pitch = 0.008;
    TS_ASSERT(toVector3d(detInfo.position(0))
                  .isApprox(toVector3d(V3D{0.0, 0.0, 5.0}), 1e-12));
    TS_ASSERT(toVector3d(detInfo.position(1))
                  .isApprox(toVector3d(V3D{0.0, scaley * pitch, 5.0}), 1e-12));
    TS_ASSERT(toVector3d(detInfo.position(2))
                  .isApprox(toVector3d(V3D{scalex * pitch, 0.0, 5.0}), 1e-12));
    TS_ASSERT(
        toVector3d(detInfo.position(3))
            .isApprox(toVector3d(V3D{scalex * pitch, scaley * pitch, 5.0}),
                      1e-12));

    const V3D detOffset{0.2, 0.3, 0.4};
    const V3D detEpsilon{5e-10, 5e-10, 5e-10};

    detInfo.setPosition(2, detInfo.position(2) + detEpsilon);
    // 2 bank parameters, det pos/rot is in DetectorInfo
    TS_ASSERT_EQUALS(pmap->size(), 2);

    const auto legacyMap = instr->makeLegacyParameterMap();

    // Legacy instrument does not support positions in ParameterMap for
    // GridDetectorPixel (parameters ignored by
    // GridDetectorPixel::getRelativePos), so we cannot support this.
    detInfo.setPosition(3, detInfo.position(3) + detOffset);
    TS_ASSERT_THROWS(instr->makeLegacyParameterMap(),
                     const std::runtime_error &);

    // 2 bank parameters + 0 det parameters
    TS_ASSERT_EQUALS(legacyMap->size(), 2);
    TS_ASSERT(!legacyMap->hasDetectorInfo(baseInstrument.get()));
    Instrument legacyInstrument(baseInstrument, legacyMap);

    TS_ASSERT_EQUALS(legacyInstrument.getDetector(4)->getPos(),
                     V3D(0.0, 0.0, 5.0));
    TS_ASSERT_EQUALS(legacyInstrument.getDetector(5)->getPos(),
                     V3D(0.0, scaley * pitch, 5.0));
    TS_ASSERT_EQUALS(legacyInstrument.getDetector(6)->getPos(),
                     V3D(scalex * pitch, 0.0, 5.0));
    TS_ASSERT_EQUALS(legacyInstrument.getDetector(7)->getPos(),
                     V3D(scalex * pitch, scaley * pitch, 5.0));
  }

  void test_empty_Instrument() {
    Instrument emptyInstrument{};
    TS_ASSERT(emptyInstrument.isEmptyInstrument());
  }

  void test_not_empty_Instrument() {

    Instrument instrument{};
    TS_ASSERT(instrument.isEmptyInstrument());
    instrument.add(new CompAssembly{});
    TS_ASSERT(!instrument.isEmptyInstrument());
  }

  void test_duplicate_detectors_throw_via_mark_as_detector() {

    // Create a very basic instrument to visit
    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        V3D(0, 0, 0) /*source pos*/, V3D(10, 0, 0) /*sample pos*/
        ,
        V3D(11, 0, 0) /*detector position*/);

    // Create an add a duplicate detector
    Detector *det =
        new Detector("invalid_detector", 1 /*DUPLICATE detector id*/, nullptr);
    instrument->add(det);
    TSM_ASSERT_THROWS("Duplicate ID, should throw",
                      instrument->markAsDetector(det), std::runtime_error &);
  }

  void test_duplicate_detectors_throw_via_mark_as_detector_finalize() {

    // Create a very basic instrument to visit
    auto instrument = ComponentCreationHelper::createMinimalInstrument(
        V3D(0, 0, 0) /*source pos*/, V3D(10, 0, 0) /*sample pos*/
        ,
        V3D(11, 0, 0) /*detector position*/);

    // Create an add a duplicate detector
    Detector *det =
        new Detector("invalid_detector", 1 /*DUPLICATE detector id*/, nullptr);
    instrument->add(det);
    instrument->markAsDetectorIncomplete(det);
    TSM_ASSERT_THROWS("Duplicate ID, should throw",
                      instrument->markAsDetectorFinalize(),
                      std::runtime_error &);
  }

private:
  Instrument_sptr createInstrumentWithSource() {
    using Mantid::Kernel::V3D;
    Instrument_sptr instr(new Instrument);
    ObjComponent *source = new ObjComponent("source");
    source->setPos(V3D(0., 0., -10.));
    instr->add(source);
    instr->markAsSource(source);

    return instr;
  }

  Instrument instrument;
  Detector *det, *det2, *det3;
};

class InstrumentTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static InstrumentTestPerformance *createSuite() {
    return new InstrumentTestPerformance();
  }
  static void destroySuite(InstrumentTestPerformance *suite) { delete suite; }

  InstrumentTestPerformance() {
    Mantid::Kernel::V3D sourcePos(0, 0, 0);
    Mantid::Kernel::V3D samplePos(0, 0, 1);
    Mantid::Kernel::V3D trolley1Pos(0, 0, 3);
    Mantid::Kernel::V3D trolley2Pos(0, 0, 6);

    m_instrumentNotParameterized = ComponentCreationHelper::sansInstrument(
        sourcePos, samplePos, trolley1Pos, trolley2Pos);

    auto map = boost::make_shared<ParameterMap>();
    m_instrumentParameterized =
        boost::make_shared<Instrument>(m_instrumentNotParameterized, map);
  }

  void test_access_pos_non_parameterized() {

    const detid_t nPixels = 100 * 100 * 6;
    double pos_x = 0;
    for (detid_t i = 1; i <= nPixels; i++) {
      pos_x += m_instrumentNotParameterized->getDetector(i)->getPos().X();
    }
  }

  void test_access_pos_parameterized() {

    const detid_t nPixels = 100 * 100 * 6;
    double pos_x = 0;
    for (detid_t i = 1; i <= nPixels; i++) {
      pos_x += m_instrumentParameterized->getDetector(i)->getPos().X();
    }
  }

  void test_access_rotate_non_parameterized() {

    const detid_t nPixels = 100 * 100 * 6;
    Kernel::Quat rot;
    for (detid_t i = 1; i <= nPixels; i++) {
      rot += m_instrumentNotParameterized->getDetector(i)->getRotation();
    }
  }

  void test_access_rotate_parameterized() {

    const detid_t nPixels = 100 * 100 * 6;
    Kernel::Quat rot;
    for (detid_t i = 1; i <= nPixels; i++) {
      rot += m_instrumentParameterized->getDetector(i)->getRotation();
    }
  }

private:
  Instrument_sptr m_instrumentParameterized;
  Instrument_sptr m_instrumentNotParameterized;
};

#endif /*INSTRUMENTTEST_H_*/
