#ifndef DETECTOREFFICIENCYCORTEST_H_
#define DETECTOREFFICIENCYCORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/DetectorEfficiencyCor.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

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

    TS_ASSERT_THROWS(corrector.execute(), std::invalid_argument);
  }

  void testDataWithUngroupedDetectors() {
    using namespace Mantid::API;
    auto inputWS = createTestWorkspace();

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
    TS_ASSERT_DELTA(result->readY(0).front(), 10.07373656, 1e-8);
    TS_ASSERT_DELTA(result->readY(0).back(), 0.0, 1e-8);
  }

  void testDataWithGroupedDetectors() {
    using namespace Mantid::API;
    // Instrument as 2 detectors but first spectrum by default only knows about
    // first one
    auto inputWS = createTestWorkspace();

    // Make it point to both detectors
    auto *spec0 = inputWS->getSpectrum(0);
    spec0->clearDetectorIDs();
    spec0->addDetectorID(1);
    spec0->addDetectorID(2);

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
    TS_ASSERT_DELTA(result->readY(0).front(), 10.07367566, 1e-8);
    TS_ASSERT_DELTA(result->readY(0).back(), 0.0, 1e-8);
  }

private:
  Mantid::API::MatrixWorkspace_sptr createTestWorkspace() {
    using namespace Mantid::Kernel;
    using namespace Mantid::API;
    using namespace Mantid::Geometry;
    using namespace Mantid::DataObjects;
    using Mantid::MantidVecPtr;

    const int nspecs(1);
    const int nbins(4);
    MatrixWorkspace_sptr space = WorkspaceFactory::Instance().create(
        "Workspace2D", nspecs, nbins + 1, nbins);
    space->getAxis(0)->unit() = UnitFactory::Instance().create("DeltaE");
    Workspace2D_sptr space2D = boost::dynamic_pointer_cast<Workspace2D>(space);

    MantidVecPtr x, y, e;
    x.access().resize(nbins + 1, 0.0);
    y.access().resize(nbins, 0.0);
    e.access().resize(nbins, 0.0);
    for (int i = 0; i < nbins; ++i) {
      x.access()[i] = static_cast<double>((1 + i) / 100);
      y.access()[i] = 10 + i;
      e.access()[i] = sqrt(5.0);
    }
    x.access()[nbins] = static_cast<double>(nbins);
    // Fill a couple of zeros just as a check that it doesn't get changed
    y.access()[nbins - 1] = 0.0;
    e.access()[nbins - 1] = 0.0;

    for (int i = 0; i < nspecs; i++) {
      space2D->setX(i, x);
      space2D->setData(i, y, e);
    }

    std::string xmlShape = "<cylinder id=\"shape\"> ";
    xmlShape += "<centre-of-bottom-base x=\"0.0\" y=\"0.0\" z=\"0.0\" /> ";
    xmlShape += "<axis x=\"0.0\" y=\"1.0\" z=\"0\" /> ";
    xmlShape += "<radius val=\"0.0127\" /> ";
    xmlShape += "<height val=\"1\" /> ";
    xmlShape += "</cylinder>";
    xmlShape += "<algebra val=\"shape\" /> ";

    // convert into a Geometry object
    ShapeFactory sFactory;
    bool addTypeTag = true;
    boost::shared_ptr<Object> shape =
        ShapeFactory().createShape(xmlShape, addTypeTag);

    boost::shared_ptr<Instrument> instrument(new Instrument);
    space2D->setInstrument(instrument);
    ObjComponent *sample = new ObjComponent("sample", shape, NULL);
    sample->setPos(0, 0, 0);
    instrument->markAsSamplePos(sample);

    ParameterMap &pmap = space2D->instrumentParameters();
    // Detector info
    const int ndets(2);
    for (int i = 0; i < ndets; ++i) {
      Detector *detector = new Detector("det", i + 1, shape, NULL);
      detector->setPos(i * 0.2, i * 0.2, 5);
      pmap.add("double", detector, "TubePressure", 10.0);
      pmap.add("double", detector, "TubeThickness", 0.0008);
      instrument->markAsDetector(detector);
    }
    return space2D;
  }
};

#endif /*DETECTOREFFICIENCYCOR_H_*/
