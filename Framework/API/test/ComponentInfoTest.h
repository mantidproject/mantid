#ifndef MANTID_API_COMPONENTINFOTEST_H_
#define MANTID_API_COMPONENTINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ComponentInfo.h"
#include "MantidBeamline/ComponentInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/ObjComponent.h"

#include <boost/make_shared.hpp>

using Mantid::API::ComponentInfo;
namespace {

boost::shared_ptr<
    const std::unordered_map<Mantid::Geometry::ComponentID, size_t>>
makeComponentIDMap(const boost::shared_ptr<
    const std::vector<Mantid::Geometry::ComponentID>> &componentIds) {
  auto idMap = boost::make_shared<
      std::unordered_map<Mantid::Geometry::ComponentID, size_t>>();

  for (size_t i = 0; i < componentIds->size(); ++i) {
    (*idMap)[(*componentIds)[i]] = i;
  }
  return idMap;
}
}

class ComponentInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created
  // statically
  // This means the constructor isn't called when running other tests
  static ComponentInfoTest *createSuite() { return new ComponentInfoTest(); }
  static void destroySuite(ComponentInfoTest *suite) { delete suite; }

  void test_size() {

    auto detectorIndices = boost::make_shared<
        std::vector<size_t>>(); // No detectors in this example
    auto ranges = boost::make_shared<std::vector<std::pair<size_t, size_t>>>();
    ranges->push_back(std::make_pair(0, 0)); // One component with no detectors
    ranges->push_back(
        std::make_pair(0, 0)); // Another component with no detectors
    Mantid::Beamline::ComponentInfo internalInfo(detectorIndices, ranges);
    Mantid::Geometry::ObjComponent comp1("component1");
    Mantid::Geometry::ObjComponent comp2("component2");

    auto componentIds =
        boost::make_shared<std::vector<Mantid::Geometry::ComponentID>>(
            std::vector<Mantid::Geometry::ComponentID>{&comp1, &comp2});
    ComponentInfo info(internalInfo, componentIds,
                       makeComponentIDMap(componentIds));
    TS_ASSERT_EQUALS(info.size(), 2);
  }

  void test_equality() {

    auto detectorIndices = boost::make_shared<
        std::vector<size_t>>(); // No detectors in this example
    auto ranges = boost::make_shared<std::vector<std::pair<size_t, size_t>>>();
    ranges->push_back(std::make_pair(0, 0)); // One component with no detectors
    ranges->push_back(
        std::make_pair(0, 0)); // Another component with no detectors
    Mantid::Beamline::ComponentInfo internalInfo(detectorIndices, ranges);
    Mantid::Geometry::ObjComponent comp1("component1");
    Mantid::Geometry::ObjComponent comp2("component2");

    auto componentIds =
        boost::make_shared<std::vector<Mantid::Geometry::ComponentID>>(
            std::vector<Mantid::Geometry::ComponentID>{&comp1, &comp2});
    ComponentInfo a(internalInfo, componentIds,
                    makeComponentIDMap(componentIds));
    auto b = a; // Copy construct. As far as we care, A & B are same.
    TS_ASSERT_EQUALS(a, b);

    // Different component id. As far as we care, A & C are NOT same.
    Mantid::Geometry::ObjComponent comp3("component3");
    componentIds =
        boost::make_shared<std::vector<Mantid::Geometry::ComponentID>>(
            std::vector<Mantid::Geometry::ComponentID>{&comp1, &comp3});
    ComponentInfo c(internalInfo, componentIds,
                    makeComponentIDMap(componentIds));
    TS_ASSERT_DIFFERS(a, c);
  }

  void test_indexOf() {

    auto detectorIndices = boost::make_shared<
        std::vector<size_t>>(); // No detectors in this example
    auto ranges = boost::make_shared<std::vector<std::pair<size_t, size_t>>>();
    ranges->push_back(std::make_pair(0, 0)); // One component with no detectors
    ranges->push_back(
        std::make_pair(0, 0)); // Another component with no detectors
    Mantid::Beamline::ComponentInfo internalInfo(detectorIndices, ranges);
    Mantid::Geometry::ObjComponent comp1("component1");
    Mantid::Geometry::ObjComponent comp2("component2");

    auto componentIds =
        boost::make_shared<std::vector<Mantid::Geometry::ComponentID>>(
            std::vector<Mantid::Geometry::ComponentID>{&comp1, &comp2});
    ComponentInfo info(internalInfo, componentIds,
                       makeComponentIDMap(componentIds));
    TS_ASSERT_EQUALS(info.indexOf(comp1.getComponentID()), 0);
    TS_ASSERT_EQUALS(info.indexOf(comp2.getComponentID()), 1);
  }

  void test_detector_indices() {

    /*
     Detectors marked with their indices
           |
     ------------
     |         | 1
    -------
    | 0  | 2
    */

    auto detectorIndices = boost::make_shared<std::vector<size_t>>(
        std::vector<size_t>{0, 2, 1}); // No detectors in this example
    auto ranges = boost::make_shared<std::vector<std::pair<size_t, size_t>>>();
    ranges->push_back(std::make_pair(0, 3));
    ranges->push_back(std::make_pair(2, 3));
    Mantid::Beamline::ComponentInfo internalInfo(detectorIndices, ranges);
    Mantid::Geometry::ObjComponent fakeComposite1("fakeComp1");
    Mantid::Geometry::ObjComponent fakeComposite2("fakeComp2");
    Mantid::Geometry::ObjComponent fakeDetector1("fakeDetector1");
    Mantid::Geometry::ObjComponent fakeDetector2("fakeDetector2");
    Mantid::Geometry::ObjComponent fakeDetector3("fakeDetector3");

    auto componentIds =
        boost::make_shared<std::vector<Mantid::Geometry::ComponentID>>(
            std::vector<Mantid::Geometry::ComponentID>{
                &fakeComposite1, &fakeComposite2, &fakeDetector1,
                &fakeDetector2, &fakeDetector3});
    ComponentInfo info(internalInfo, componentIds,
                       makeComponentIDMap(componentIds));
    TS_ASSERT_EQUALS(info.detectorIndices(3 /*component index*/),
                     std::vector<size_t>({0, 2, 1}));
    TS_ASSERT_EQUALS(info.detectorIndices(4 /*component index*/),
                     std::vector<size_t>({1}));
  }
};

#endif /* MANTID_API_COMPONENTINFOTEST_H_ */
