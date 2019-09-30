// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef DETECTOREFFICIENCYCORTEST_H_
#define DETECTOREFFICIENCYCORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Axis.h"
#include "MantidAlgorithms/DetectorEfficiencyCor.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::HistogramData;

class DetectorEfficiencyCorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DetectorEfficiencyCorTest *createSuite() {
    return new DetectorEfficiencyCorTest();
  }
  static void destroySuite(DetectorEfficiencyCorTest *suite) { delete suite; }

  void testInit() {
    Mantid::Algorithms::DetectorEfficiencyCor grouper;
    TS_ASSERT_EQUALS(grouper.name(), "DetectorEfficiencyCor");
    TS_ASSERT_EQUALS(grouper.version(), 1);
    TS_ASSERT_THROWS_NOTHING(grouper.initialize());
    TS_ASSERT(grouper.isInitialized());
  }

  void testExecWithoutEiThrowsInvalidArgument() {
    auto dummyWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 1);
    dummyWS->getAxis(0)->unit() =
        Mantid::Kernel::UnitFactory::Instance().create("DeltaE");
    const std::string inputWS = "testInput";

    Mantid::Algorithms::DetectorEfficiencyCor corrector;
    TS_ASSERT_THROWS_NOTHING(corrector.initialize());
    TS_ASSERT(corrector.isInitialized());
    corrector.setChild(true);
    corrector.setRethrows(true);

    corrector.setProperty("InputWorkspace", dummyWS);
    corrector.setPropertyValue("OutputWorkspace", "__unused");

    TS_ASSERT_THROWS(corrector.execute(), const std::invalid_argument &);
  }

  void testDataWithUngroupedDetectors() {
    using namespace Mantid::API;
    auto inputWS = createTestWorkspace();
    inputWS->getSpectrum(0).setDetectorID(1);

    Mantid::Algorithms::DetectorEfficiencyCor grouper;
    TS_ASSERT_THROWS_NOTHING(grouper.initialize());
    TS_ASSERT(grouper.isInitialized());
    grouper.setChild(true);

    TS_ASSERT_THROWS_NOTHING(grouper.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        grouper.setPropertyValue("OutputWorkspace", "__unused"));
    TS_ASSERT_THROWS_NOTHING(grouper.setProperty("IncidentEnergy", 2.1));
    grouper.execute();
    TS_ASSERT(grouper.isExecuted());

    MatrixWorkspace_sptr result = grouper.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(result->getNumberHistograms(), 1);
    TS_ASSERT_DELTA(result->y(0).front(), 10.07373656, 1e-8);
    TS_ASSERT_DELTA(result->y(0).back(), 0.0, 1e-8);
  }

  void testDataWithGroupedDetectors() {
    using namespace Mantid::API;
    // Instrument as 2 detectors but first spectrum by default only knows about
    // first one
    auto inputWS = createTestWorkspace();

    // Make it point to both detectors
    auto &spec0 = inputWS->getSpectrum(0);
    spec0.clearDetectorIDs();
    spec0.addDetectorID(1);
    spec0.addDetectorID(2);

    Mantid::Algorithms::DetectorEfficiencyCor grouper;
    TS_ASSERT_THROWS_NOTHING(grouper.initialize());
    TS_ASSERT(grouper.isInitialized());
    grouper.setChild(true);

    TS_ASSERT_THROWS_NOTHING(grouper.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(
        grouper.setPropertyValue("OutputWorkspace", "__unused"));
    TS_ASSERT_THROWS_NOTHING(grouper.setProperty("IncidentEnergy", 2.1));
    grouper.execute();
    TS_ASSERT(grouper.isExecuted());

    MatrixWorkspace_sptr result = grouper.getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(result->getNumberHistograms(), 1);
    TS_ASSERT_DELTA(result->y(0).front(), 10.07367566, 1e-8);
    TS_ASSERT_DELTA(result->y(0).back(), 0.0, 1e-8);
  }

private:
  Mantid::API::MatrixWorkspace_sptr createTestWorkspace() {
    using namespace Mantid::Kernel;
    using namespace Mantid::API;
    using namespace Mantid::Geometry;
    using namespace Mantid::DataObjects;

    std::string xmlShape = "<cylinder id=\"shape\"> ";
    xmlShape += R"(<centre-of-bottom-base x="0.0" y="0.0" z="0.0" /> )";
    xmlShape += R"(<axis x="0.0" y="1.0" z="0" /> )";
    xmlShape += "<radius val=\"0.0127\" /> ";
    xmlShape += "<height val=\"1\" /> ";
    xmlShape += "</cylinder>";
    xmlShape += "<algebra val=\"shape\" /> ";

    // convert into a Geometry object
    bool addTypeTag = true;
    auto shape = ShapeFactory().createShape(xmlShape, addTypeTag);

    boost::shared_ptr<Instrument> instrument = boost::make_shared<Instrument>();
    const int ndets(2);
    std::vector<Detector *> detectors;
    for (int i = 0; i < ndets; ++i) {
      Detector *detector = new Detector("det", i + 1, shape, nullptr);
      detector->setPos(i * 0.2, i * 0.2, 5);
      instrument->add(detector);
      instrument->markAsDetector(detector);
      detectors.push_back(detector);
    }
    ObjComponent *sample = new ObjComponent("sample", shape, nullptr);
    sample->setPos(0, 0, 0);
    instrument->markAsSamplePos(sample);

    const int nspecs(1);
    auto space2D = create<Workspace2D>(
        instrument, nspecs,
        Histogram(BinEdges{1e-14, 2e-14, 3e-14, 4e-14, 4.0},
                  Counts{10, 11, 12, 0}, CountVariances{5.0, 5.0, 5.0, 0.0}));
    space2D->getAxis(0)->unit() = UnitFactory::Instance().create("DeltaE");

    ParameterMap &pmap = space2D->instrumentParameters();
    for (const auto detector : detectors) {
      pmap.add("double", detector, "TubePressure", 10.0);
      pmap.add("double", detector, "TubeThickness", 0.0008);
    }
    return std::move(space2D);
  }
};

#endif /*DETECTOREFFICIENCYCOR_H_*/
