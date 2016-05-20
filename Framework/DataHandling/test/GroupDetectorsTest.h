#ifndef GROUPDETECTORSTEST_H_
#define GROUPDETECTORSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/GroupDetectors.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"

using Mantid::DataHandling::GroupDetectors;
using Mantid::MantidVecPtr;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using Mantid::detid_t;
using Mantid::specnum_t;
using Mantid::HistogramData::BinEdges;

class GroupDetectorsTest : public CxxTest::TestSuite {
public:
  static GroupDetectorsTest *createSuite() { return new GroupDetectorsTest(); }
  static void destroySuite(GroupDetectorsTest *suite) { delete suite; }

  GroupDetectorsTest() {
    // Set up a small workspace for testing
    MatrixWorkspace_sptr space =
        WorkspaceFactory::Instance().create("Workspace2D", 5, 6, 5);
    space->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);
    BinEdges x(6, 10.0);
    MantidVecPtr vec;
    vec.access().resize(5, 1.0);
    for (int j = 0; j < 5; ++j) {
      space2D->setBinEdges(j, x);
      space2D->setData(j, vec, vec);
      space2D->getSpectrum(j)->setSpectrumNo(j);
      space2D->getSpectrum(j)->setDetectorID(j);
    }
    Instrument_sptr instr(new Instrument);
    for (detid_t i = 0; i < 5; i++) {
      Detector *d = new Detector("det", i, 0);
      instr->markAsDetector(d);
    }
    space->setInstrument(instr);

    // Register the workspace in the data service
    AnalysisDataService::Instance().add("GroupTestWS", space);
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
    std::vector<double> tens(6, 10.0);
    std::vector<double> ones(5, 1.0);
    std::vector<double> threes(5, 3.0);
    std::vector<double> zeroes(5, 0.0);
    TS_ASSERT_EQUALS(outputWS->dataX(0), tens);
    TS_ASSERT_EQUALS(outputWS->dataY(0), threes);
    for (int i = 0; i < 5; ++i) {
      TS_ASSERT_DELTA(outputWS->dataE(0)[i], 1.7321, 0.0001);
    }
    TS_ASSERT_EQUALS(outputWS->getSpectrum(0)->getSpectrumNo(), 0);
    TS_ASSERT_EQUALS(outputWS->dataX(1), tens);
    TS_ASSERT_EQUALS(outputWS->dataY(1), ones);
    TS_ASSERT_EQUALS(outputWS->dataE(1), ones);
    TS_ASSERT_EQUALS(outputWS->getSpectrum(1)->getSpectrumNo(), 1);
    TS_ASSERT_EQUALS(outputWS->dataX(2), tens);
    TS_ASSERT_EQUALS(outputWS->dataY(2), zeroes);
    TS_ASSERT_EQUALS(outputWS->dataE(2), zeroes);
    TS_ASSERT_EQUALS(outputWS->getSpectrum(2)->getSpectrumNo(), -1);
    TS_ASSERT_EQUALS(outputWS->dataX(3), tens);
    TS_ASSERT_EQUALS(outputWS->dataY(3), zeroes);
    TS_ASSERT_EQUALS(outputWS->dataE(3), zeroes);
    TS_ASSERT_EQUALS(outputWS->getSpectrum(3)->getSpectrumNo(), -1);
    TS_ASSERT_EQUALS(outputWS->dataX(4), tens);
    TS_ASSERT_EQUALS(outputWS->dataY(4), ones);
    TS_ASSERT_EQUALS(outputWS->dataE(4), ones);
    TS_ASSERT_EQUALS(outputWS->getSpectrum(4)->getSpectrumNo(), 4);

    boost::shared_ptr<const IDetector> det;
    TS_ASSERT_THROWS_NOTHING(det = outputWS->getDetector(0));
    TS_ASSERT(boost::dynamic_pointer_cast<const DetectorGroup>(det));
    TS_ASSERT_THROWS_NOTHING(det = outputWS->getDetector(1));
    TS_ASSERT(boost::dynamic_pointer_cast<const Detector>(det));
    TS_ASSERT_THROWS(outputWS->getDetector(2), Exception::NotFoundError);
    TS_ASSERT_THROWS(outputWS->getDetector(3), Exception::NotFoundError);
    TS_ASSERT_THROWS_NOTHING(det = outputWS->getDetector(4));
    TS_ASSERT(boost::dynamic_pointer_cast<const Detector>(det));
    AnalysisDataService::Instance().remove("GroupTestWS");
  }

private:
  GroupDetectors grouper;
};

#endif /*GROUPDETECTORSTEST_H_*/
