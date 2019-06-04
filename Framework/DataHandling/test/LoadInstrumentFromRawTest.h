// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef LOADINSTRUMENTTESTFROMRAW_H_
#define LOADINSTRUMENTTESTFROMRAW_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadInstrumentFromRaw.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadInstrumentFromRawTest : public CxxTest::TestSuite {
public:
  static LoadInstrumentFromRawTest *createSuite() {
    return new LoadInstrumentFromRawTest();
  }
  static void destroySuite(LoadInstrumentFromRawTest *suite) { delete suite; }

  LoadInstrumentFromRawTest() {}

  void testInit() {
    TS_ASSERT(!loader.isInitialized());
    TS_ASSERT_THROWS_NOTHING(loader.initialize());
    TS_ASSERT(loader.isInitialized());
  }

  void testExecHET() {
    if (!loader.isInitialized())
      loader.initialize();

    // create a workspace with some sample data
    wsName = "LoadInstrumentFromRawTest";
    Workspace_sptr ws =
        WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    // put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // set properties and check this are set ok
    loader.setPropertyValue("Filename", "LOQ48127.raw");
    inputFile = loader.getPropertyValue("Filename");
    loader.setPropertyValue("Workspace", wsName);

    std::string result;
    TS_ASSERT_THROWS_NOTHING(result = loader.getPropertyValue("Filename"))
    TS_ASSERT(!result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING(result = loader.getPropertyValue("Workspace"))
    TS_ASSERT(!result.compare(wsName));

    // execute
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    // Get back the saved workspace
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            wsName));

    boost::shared_ptr<const Instrument> i = output->getInstrument();
    TS_ASSERT_EQUALS(i->getName(), "LOQ     ");
    boost::shared_ptr<const IComponent> source = i->getSource();
    TS_ASSERT_EQUALS(source->getName(), "Source");
    TS_ASSERT_DELTA(source->getPos().Z(), -11, 0.01);

    boost::shared_ptr<const IComponent> samplepos = i->getSample();
    TS_ASSERT_DELTA(samplepos->getPos().Y(), 0.0, 0.01);

    const auto &detectorInfo = output->detectorInfo();
    const auto &ptrDetSp = detectorInfo.detector(detectorInfo.indexOf(5));
    TS_ASSERT_EQUALS(ptrDetSp.getID(), 5);
    TS_ASSERT_EQUALS(ptrDetSp.getName(), "det");
    TS_ASSERT_EQUALS(ptrDetSp.type(), "DetectorComponent");
    // using phi values from raw file changes sign of this
    TS_ASSERT_DELTA(ptrDetSp.getPos().X(), 0, 0.01);
    TS_ASSERT_DELTA(ptrDetSp.getPos().Z(), -11.1499, 0.01);
    TS_ASSERT_DELTA(detectorInfo.l2(detectorInfo.indexOf(5)), 11.1499, 0.0001);

    // also a few tests on the last detector and a test for the one beyond the
    // last
    const auto &ptrDetLast = detectorInfo.detector(detectorInfo.indexOf(8));
    TS_ASSERT_EQUALS(ptrDetLast.getID(), 8);
    TS_ASSERT_THROWS(detectorInfo.indexOf(9), const std::out_of_range &);

    // Check the monitors are correctly marked
    const auto &detInfo = output->detectorInfo();
    TS_ASSERT(detInfo.isMonitor(0))
    TS_ASSERT(detInfo.isMonitor(1))
    // ...and that a normal detector isn't
    TS_ASSERT(!detInfo.isMonitor(2))
    TS_ASSERT(!detInfo.isMonitor(3))
    TS_ASSERT(!detInfo.isMonitor(7))

    AnalysisDataService::Instance().remove(wsName);
  }

private:
  LoadInstrumentFromRaw loader;
  std::string inputFile;
  std::string wsName;
};

#endif /*LOADINSTRUMENTTESTFROMRAW_H_*/
