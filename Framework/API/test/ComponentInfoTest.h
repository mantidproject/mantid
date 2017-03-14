#ifndef MANTID_API_COMPONENTINFOTEST_H_
#define MANTID_API_COMPONENTINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ComponentInfo.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/ObjComponent.h"

using Mantid::API::ComponentInfo;

class ComponentInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ComponentInfoTest *createSuite() { return new ComponentInfoTest(); }
  static void destroySuite(ComponentInfoTest *suite) { delete suite; }

  void test_size() {
    std::vector<std::vector<size_t>> detectorIndexes{
        {}, {}}; // No detectors in this example
    Mantid::Beamline::ComponentInfo internalInfo(detectorIndexes);
    Mantid::Geometry::ObjComponent comp1("component1");
    Mantid::Geometry::ObjComponent comp2("component2");
    ComponentInfo info(internalInfo, std::vector<Mantid::Geometry::ComponentID>{
                                         &comp1, &comp2});
    TS_ASSERT_EQUALS(info.size(), 2);
  }

  void test_indexOf() {
    std::vector<std::vector<size_t>> detectorIndexes{
        {}, {}}; // No detectors in this example
    Mantid::Beamline::ComponentInfo internalInfo(detectorIndexes);
    Mantid::Geometry::ObjComponent comp1("component1");
    Mantid::Geometry::ObjComponent comp2("component2");
    ComponentInfo info(internalInfo, std::vector<Mantid::Geometry::ComponentID>{
                                         &comp1, &comp2});
    TS_ASSERT_EQUALS(info.indexOf(comp1.getComponentID()), 0);
    TS_ASSERT_EQUALS(info.indexOf(comp2.getComponentID()), 1);
  }

  void test_throws_if_componentids_and_internalinfo_size_mismatch() {
    std::vector<std::vector<size_t>> detectorIndexes{
        {}, {}}; // No detectors in this example
    Mantid::Beamline::ComponentInfo internalInfo(detectorIndexes);
    TSM_ASSERT_THROWS(
        "Should throw too few ComponentIDs",
        ComponentInfo(internalInfo,
                      std::vector<Mantid::Geometry::ComponentID>{}),
        std::invalid_argument &);
  }

  void test_detector_indexes() {

    /*
           |
     ------------
     |         | 30
    -------
    | 10  | 20
    */

    std::vector<std::vector<size_t>> indexes{
        {10, 20, 30}, {10, 20}, {}, {}, {}};
    Mantid::Beamline::ComponentInfo internalInfo(indexes);
    Mantid::Geometry::ObjComponent fakeComposite1("fakeComp1");
    Mantid::Geometry::ObjComponent fakeComposite2("fakeComp2");
    Mantid::Geometry::ObjComponent fakeDetector1("fakeDetector1");
    Mantid::Geometry::ObjComponent fakeDetector2("fakeDetector2");
    Mantid::Geometry::ObjComponent fakeDetector3("fakeDetector3");

    ComponentInfo info(internalInfo,
                       std::vector<Mantid::Geometry::ComponentID>{
                           &fakeComposite1, &fakeComposite2, &fakeDetector1,
                           &fakeDetector2, &fakeDetector3});
    TS_ASSERT_EQUALS(info.detectorIndexes(0 /*component index*/),
                     std::vector<size_t>({10, 20, 30}));
    TS_ASSERT_EQUALS(info.detectorIndexes(1 /*component index*/),
                     std::vector<size_t>({10, 20}));
  }
};

#endif /* MANTID_API_COMPONENTINFOTEST_H_ */
