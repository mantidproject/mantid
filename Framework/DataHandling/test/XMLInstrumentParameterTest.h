#ifndef XMLLOGFILETEST_H_
#define XMLLOGFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/XMLInstrumentParameter.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class XMLInstrumentParameterTest : public CxxTest::TestSuite {
public:
  // LoadRaw2 uses XMLInstrumentParameter to populate its parameter map. Hence
  // the test here simply
  // checks that this is done ok
  void testParameterMap() {
    LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "CSP79590.raw");
    loader.setPropertyValue("OutputWorkspace", "CRISPdata");

    TS_ASSERT_THROWS_NOTHING(loader.execute())
    TS_ASSERT(loader.isExecuted())

    // Get back the workspaces
    MatrixWorkspace_sptr output1;
    TS_ASSERT_THROWS_NOTHING(
        output1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "CRISPdata_1"));
    TS_ASSERT_EQUALS(output1->getNumberHistograms(), 4)
    MatrixWorkspace_sptr output2;
    TS_ASSERT_THROWS_NOTHING(
        output2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "CRISPdata_2"));
    TS_ASSERT_EQUALS(output2->getNumberHistograms(), 4)

    // get the parameter map for the period 1 CRISP data
    const auto &paramMap = output1->constInstrumentParameters();

    // check that parameter have been read into the instrument parameter map
    const auto &detectorInfo = output1->detectorInfo();
    const auto pos1 = detectorInfo.position(2); // ID 3 -> index 2
    std::vector<V3D> ret1 = paramMap.getV3D("point-detector", "pos");
    // point-detector is a single detector, its position is stored in
    // DetectorInfo, parameter has been purged from the map
    TS_ASSERT_EQUALS(static_cast<int>(ret1.size()), 0);
    TS_ASSERT_DELTA(pos1.Z(), 12.113, 0.0001);
    TS_ASSERT_DELTA(pos1.X(), 0.0, 0.0001);
    TS_ASSERT_DELTA(pos1.Y(), 0.0162, 0.0001);
    // linear-detector is composite, i.e., not a detector and thus not stored in
    // DetectorInfo but in ComponentInfo
    const auto &componentInfo = output1->componentInfo();
    const auto &linearDetector = output1->getInstrument()
                                     ->getComponentByName("linear-detector")
                                     ->getComponentID();
    const auto pos2 =
        componentInfo.position(componentInfo.indexOf(linearDetector));
    std::vector<V3D> ret2 = paramMap.getV3D("linear-detector", "pos");
    TS_ASSERT_EQUALS(static_cast<int>(ret2.size()), 0);
    TS_ASSERT_DELTA(pos2.Y(), 0.0, 0.0001);
    TS_ASSERT_DELTA(pos2.X(), 0.0, 0.0001);
    TS_ASSERT_DELTA(pos2.Z(), 0.1499, 0.0001);
    std::vector<double> ret3 = paramMap.getDouble("slit1", "vertical gap");
    TS_ASSERT_EQUALS(static_cast<int>(ret3.size()), 1);
    TS_ASSERT_DELTA(ret3[0], 0.5005, 0.0001);
  }

  // LoadRaw2 uses XMLInstrumentParameter to populate its parameter map. Hence
  // the test here simply
  // checks that this is done ok
  void testParsing() {
    IComponent *comp(nullptr);
    boost::shared_ptr<Interpolation> interpolation =
        boost::make_shared<Interpolation>();
    std::vector<std::string> constraint;
    std::string penaltyFactor;
    std::string fitFunc;
    std::string extractSingleValueAs;
    std::string eq;
    const double angleConvert(1.0);

    XMLInstrumentParameter testParamEntry(
        "", "1000.0", interpolation, "", "", "", "bob", "double", "",
        constraint, penaltyFactor, fitFunc, extractSingleValueAs, eq, comp,
        angleConvert, "bla bla bla");

    TimeSeriesProperty<double> *dummy = nullptr;
    TS_ASSERT_DELTA(testParamEntry.createParamValue(dummy), 1000.0, 0.0001);

    interpolation->addPoint(201.0, 60);
    TS_ASSERT_DELTA(testParamEntry.createParamValue(dummy), 0.0, 0.0001);
  }
};

#endif /*XMLLOGFILETEST_H_*/
