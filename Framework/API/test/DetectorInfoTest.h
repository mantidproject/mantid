#ifndef MANTID_API_DETECTORINFOTEST_H_
#define MANTID_API_DETECTORINFOTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/DetectorInfo.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MantidTestHelpers/InstrumentCreationHelper.h"

#include <algorithm>

using namespace Mantid::Geometry;
using namespace Mantid::API;

class DetectorInfoTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetectorInfoTest *createSuite() { return new DetectorInfoTest(); }
  static void destroySuite(DetectorInfoTest *suite) { delete suite; }

  DetectorInfoTest() : m_workspace(nullptr) {
    int32_t numberOfHistograms = 5;
    size_t numberOfBins = 1;
    m_workspace.init(numberOfHistograms, numberOfBins, numberOfBins - 1);
    for (int32_t i = 0; i < numberOfHistograms; ++i)
      m_workspace.getSpectrum(i)
          .setSpectrumNo(static_cast<int32_t>(numberOfHistograms) - i);
    bool includeMonitors = false;
    bool startYNegative = true;
    const std::string instrumentName("SimpleFakeInstrument");
    InstrumentCreationHelper::addFullInstrumentToWorkspace(
        m_workspace, includeMonitors, startYNegative, instrumentName);
  }

  void test_constructor() {
    TS_ASSERT_THROWS_NOTHING(DetectorInfo(*m_workspace.getInstrument()));
  }

  void test_detectorIDs() {
    // Check that workspace does not have sorted IDs
    TS_ASSERT_EQUALS(m_workspace.getDetector(0)->getID(), 5);
    TS_ASSERT_EQUALS(m_workspace.getDetector(1)->getID(), 4);
    TS_ASSERT_EQUALS(m_workspace.getDetector(2)->getID(), 3);
    TS_ASSERT_EQUALS(m_workspace.getDetector(3)->getID(), 2);
    TS_ASSERT_EQUALS(m_workspace.getDetector(4)->getID(), 1);
    const DetectorInfo info(*m_workspace.getInstrument());
    const auto &ids = info.detectorIDs();
    auto sorted_ids(ids);
    std::sort(sorted_ids.begin(), sorted_ids.end());
    TS_ASSERT_EQUALS(ids, sorted_ids);
  }

private:
  WorkspaceTester m_workspace;
};

#endif /* MANTID_API_DETECTORINFOTEST_H_ */
