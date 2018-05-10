#ifndef GROUPDETECTORSTEST_H_
#define GROUPDETECTORSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataHandling/GroupDetectors.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/HistogramDataTestHelper.h"

using Mantid::DataHandling::GroupDetectors;
using Mantid::MantidVecPtr;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using Mantid::detid_t;
using Mantid::specnum_t;

class GroupDetectorsTest : public CxxTest::TestSuite {
public:
  static GroupDetectorsTest *createSuite() { return new GroupDetectorsTest(); }
  static void destroySuite(GroupDetectorsTest *suite) { delete suite; }

  GroupDetectorsTest() {
    // Set up a small workspace for testing
    auto space2D = createWorkspace<Workspace2D>(5, 6, 5);
    space2D->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    BinEdges x(6, LinearGenerator(10.0, 1.0));
    Counts y(5, 1.0);
    CountStandardDeviations e(5, 1.0);
    for (int j = 0; j < 5; ++j) {
      space2D->setBinEdges(j, x);
      space2D->setCounts(j, y);
      space2D->setCountStandardDeviations(j, e);
      space2D->getSpectrum(j).setSpectrumNo(j);
      space2D->getSpectrum(j).setDetectorID(j);
    }
    Instrument_sptr instr(new Instrument);
    for (detid_t i = 0; i < 5; i++) {
      Detector *d = new Detector("det", i, nullptr);
      instr->add(d);
      instr->markAsDetector(d);
    }
    space2D->setInstrument(instr);

    // Register the workspace in the data service
    AnalysisDataService::Instance().add("GroupTestWS", space2D);
  }

  void testName() { TS_ASSERT_EQUALS(grouper.name(), "GroupDetectors") }

  void testVersion() { TS_ASSERT_EQUALS(grouper.version(), 1) }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(grouper.initialize())
    TS_ASSERT(grouper.isInitialized());

    GroupDetectors gd;
    TS_ASSERT_THROWS_NOTHING(gd.initialize())
    TS_ASSERT(gd.isInitialized());

    std::vector<Property *> props = gd.getProperties();
    TS_ASSERT_EQUALS(static_cast<int>(props.size()), 5)

    TS_ASSERT_EQUALS(props[0]->name(), "Workspace")
    TS_ASSERT(props[0]->isDefault())
    TS_ASSERT(dynamic_cast<WorkspaceProperty<> *>(props[0]))

    TS_ASSERT_EQUALS(props[1]->name(), "SpectraList")
    TS_ASSERT(props[1]->isDefault())
    TS_ASSERT(dynamic_cast<ArrayProperty<specnum_t> *>(props[1]))

    TS_ASSERT_EQUALS(props[2]->name(), "DetectorList")
    TS_ASSERT(props[2]->isDefault())
    TS_ASSERT(dynamic_cast<ArrayProperty<detid_t> *>(props[2]))

    TS_ASSERT_EQUALS(props[3]->name(), "WorkspaceIndexList")
    TS_ASSERT(props[3]->isDefault())
    TS_ASSERT(dynamic_cast<ArrayProperty<size_t> *>(props[3]))
  }

  void testExec() {
    if (!grouper.isInitialized())
      grouper.initialize();

    grouper.setPropertyValue("Workspace", "GroupTestWS");
    TS_ASSERT_THROWS_NOTHING(grouper.execute());
    TS_ASSERT(grouper.isExecuted());

    grouper.setPropertyValue("Workspace", "GroupTestWS");
    grouper.setPropertyValue("WorkspaceIndexList", "0,2");
    TS_ASSERT_THROWS_NOTHING(grouper.execute());
    TS_ASSERT(grouper.isExecuted());

    GroupDetectors grouper2;
    grouper2.initialize();
    grouper2.setPropertyValue("Workspace", "GroupTestWS");
    grouper2.setPropertyValue("SpectraList", "0,3");
    TS_ASSERT_THROWS_NOTHING(grouper2.execute());
    TS_ASSERT(grouper2.isExecuted());

    MatrixWorkspace_sptr outputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "GroupTestWS");
    const HistogramX tens{10, 11, 12, 13, 14, 15};
    const HistogramY yOnes(5, 1.0);
    const HistogramE eOnes(5, 1.0);
    const HistogramY threes(5, 3.0);
    const HistogramY yZeroes(5, 0.0);
    const HistogramE eZeroes(5, 0.0);
    TS_ASSERT_EQUALS(outputWS->x(0), tens);
    TS_ASSERT_EQUALS(outputWS->y(0), threes);
    for (int i = 0; i < 5; ++i) {
      TS_ASSERT_DELTA(outputWS->e(0)[i], 1.7321, 0.0001);
    }
    TS_ASSERT_EQUALS(outputWS->getSpectrum(0).getSpectrumNo(), 0);
    TS_ASSERT_EQUALS(outputWS->x(1), tens);
    TS_ASSERT_EQUALS(outputWS->y(1), yOnes);
    TS_ASSERT_EQUALS(outputWS->e(1), eOnes);
    TS_ASSERT_EQUALS(outputWS->getSpectrum(1).getSpectrumNo(), 1);
    TS_ASSERT_EQUALS(outputWS->x(2), tens);
    TS_ASSERT_EQUALS(outputWS->y(2), yZeroes);
    TS_ASSERT_EQUALS(outputWS->e(2), eZeroes);
    TS_ASSERT_EQUALS(outputWS->getSpectrum(2).getSpectrumNo(), -1);
    TS_ASSERT_EQUALS(outputWS->x(3), tens);
    TS_ASSERT_EQUALS(outputWS->y(3), yZeroes);
    TS_ASSERT_EQUALS(outputWS->e(3), eZeroes);
    TS_ASSERT_EQUALS(outputWS->getSpectrum(3).getSpectrumNo(), -1);
    TS_ASSERT_EQUALS(outputWS->x(4), tens);
    TS_ASSERT_EQUALS(outputWS->y(4), yOnes);
    TS_ASSERT_EQUALS(outputWS->e(4), eOnes);
    TS_ASSERT_EQUALS(outputWS->getSpectrum(4).getSpectrumNo(), 4);

    const auto &spectrumInfo = outputWS->spectrumInfo();
    TS_ASSERT(spectrumInfo.hasDetectors(0));
    const auto &det0 = spectrumInfo.detector(0);
    // (void) avoids a compiler warning for unused variable
    TS_ASSERT_THROWS_NOTHING((void)dynamic_cast<const DetectorGroup &>(det0));

    TS_ASSERT(spectrumInfo.hasDetectors(1));
    const auto &det1 = spectrumInfo.detector(1);
    TS_ASSERT_THROWS_NOTHING((void)dynamic_cast<const Detector &>(det1));

    TS_ASSERT(!spectrumInfo.hasDetectors(2));
    TS_ASSERT(!spectrumInfo.hasDetectors(3));

    TS_ASSERT(spectrumInfo.hasDetectors(4));
    const auto &det4 = spectrumInfo.detector(4);
    TS_ASSERT_THROWS_NOTHING((void)dynamic_cast<const Detector &>(det4));

    AnalysisDataService::Instance().remove("GroupTestWS");
  }

private:
  GroupDetectors grouper;
};

#endif /*GROUPDETECTORSTEST_H_*/
