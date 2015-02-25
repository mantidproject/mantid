#include <cxxtest/TestSuite.h>
#include "MantidMDEvents/IntegrateEllipsoids.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeakShapeEllipsoid.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/NearestNeighbours.h"
#include <boost/make_shared.hpp>
#include <boost/tuple/tuple.hpp>

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;

class IntegrateEllipsoidsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegrateEllipsoidsTest *createSuite() {
    return new IntegrateEllipsoidsTest();
  }

  IntegrateEllipsoidsTest()
  {
      // Because otherwise PreprocessDectectorsToMD cannot be found!
      Mantid::API::FrameworkManager::Instance();
  }

  static void destroySuite(IntegrateEllipsoidsTest *suite) { delete suite; }

  static ISpectrumDetectorMapping buildSpectrumDetectorMapping(const specid_t start, const specid_t end)
  {
    boost::unordered_map<specid_t, std::set<detid_t>> map;
    for ( specid_t i = start; i <= end; ++i )
    {
      map[i].insert(i);
    }
    return map;
  }

  void addFakeEllipsoid(const V3D& peakHKL, const int& totalNPixels, const int& nEvents, const NearestNeighbours& nn, EventWorkspace_sptr& eventWS, PeaksWorkspace_sptr& peaksWS)
  {
      // Create the peak and add it to the peaks ws
      Peak* peak = peaksWS->createPeakHKL(peakHKL);
      peaksWS->addPeak(*peak);
      const double detectorId = peak->getDetectorID();
      const double tofExact = peak->getTOF();
      delete peak;

      EventList& el = eventWS->getEventList(detectorId - totalNPixels);
      el.setDetectorID(detectorId);
      el.addEventQuickly(TofEvent(tofExact));

      // Find some neighbours,
      std::map<specid_t, Mantid::Kernel::V3D> neighbourMap = nn.neighbours(detectorId);

      typedef std::map<specid_t, Mantid::Kernel::V3D> NeighbourMap;
      typedef NeighbourMap::iterator NeighbourMapIterator;
      for(NeighbourMapIterator it = neighbourMap.begin(); it != neighbourMap.end(); ++it)
      {
          const specid_t neighbourDet = (*it).first;
          const double distanceFromCentre = (*it).second.norm2(); // gives TOF delta
          EventList neighbourEventList = eventWS->getEventList(neighbourDet - totalNPixels);
          neighbourEventList.setDetectorID(neighbourDet);
          for(int i = 0; i < nEvents; ++i) {

              const double tof = (tofExact - (distanceFromCentre/2) ) + ( distanceFromCentre * double(i)/double(nEvents) );
              neighbourEventList.addEventQuickly(TofEvent(tof));
          }
      }
  }

  boost::tuple<EventWorkspace_sptr, PeaksWorkspace_sptr> createDiffractionData()
  {
      const int nPixels = 100;
      Mantid::Geometry::Instrument_sptr inst =
      ComponentCreationHelper::createTestInstrumentRectangular(1 /*num_banks*/, nPixels /*pixels in each direction yields n by n*/,0.01, 1.0);

      // Create a peaks workspace
      auto peaksWS = boost::make_shared<PeaksWorkspace>();
      // Set the instrument to be the fake rectangular bank above.
      peaksWS->setInstrument(inst);
      // Set the oriented lattice for a cubic crystal
      OrientedLattice ol(6,6,6,90,90,90);
      ol.setUFromVectors(V3D(6, 0, 0), V3D(0, 6, 0));
      peaksWS->mutableSample().setOrientedLattice(&ol);


      // Make an event workspace and add fake peak data
      auto eventWS = boost::make_shared<EventWorkspace>();
      eventWS->setInstrument(inst);
      eventWS->initialize(nPixels * nPixels /*n spectra*/, 3 /* x-size */, 3 /* y-size */);
      eventWS->getAxis(0)->setUnit("TOF");
      // Give the spectra-detector mapping for all event lists
      const int nPixelsTotal = nPixels*nPixels;
      for(int i = 0; i < nPixelsTotal; ++i) {
        EventList& el = eventWS->getOrAddEventList(i);
        el.setDetectorID(i + nPixelsTotal);
      }

      // Make a nn map, so that we can add counts in the vicinity of the actual peak centre.
      NearestNeighbours nn(inst, buildSpectrumDetectorMapping(nPixelsTotal, nPixelsTotal+inst->getNumberDetectors() - 1));

      // Add some peaks which should correspond to real reflections (could calculate these). Same function also adds a fake ellipsoid
      addFakeEllipsoid(V3D(1, -5, -3), nPixelsTotal, 10, nn, eventWS, peaksWS);
      addFakeEllipsoid(V3D(1, -4, -4), nPixelsTotal, 10, nn, eventWS, peaksWS);
      addFakeEllipsoid(V3D(1, -3, -5), nPixelsTotal, 10, nn, eventWS, peaksWS);
      addFakeEllipsoid(V3D(1, -4, -1), nPixelsTotal, 10, nn, eventWS, peaksWS);
      addFakeEllipsoid(V3D(1, -4,  0), nPixelsTotal, 10, nn, eventWS, peaksWS);
      addFakeEllipsoid(V3D(2, -3,  -4), nPixelsTotal, 10, nn, eventWS, peaksWS);

      // Return test data.
      return boost::tuple<EventWorkspace_sptr, PeaksWorkspace_sptr>(eventWS, peaksWS);
  }

  void test_init()
  {
      Mantid::MDEvents::IntegrateEllipsoids alg;
      TS_ASSERT_THROWS_NOTHING(alg.initialize());
  }

  void test_execution_events()
  {
      auto out = createDiffractionData();
      EventWorkspace_sptr eventWS = out.get<0>();
      PeaksWorkspace_sptr peaksWS = out.get<1>();

      IntegrateEllipsoids alg;
      alg.setChild(true);
      alg.setRethrows(true);
      alg.initialize();
      alg.setProperty("InputWorkspace", eventWS);
      alg.setProperty("PeaksWorkspace", peaksWS);
      alg.setPropertyValue("OutputWorkspace", "dummy");
      alg.execute();
      PeaksWorkspace_sptr integratedPeaksWS = alg.getProperty("OutputWorkspace");
      TSM_ASSERT_EQUALS("Wrong number of peaks in output workspace", integratedPeaksWS->getNumberPeaks(), peaksWS->getNumberPeaks());
      const Peak& firstPeak = integratedPeaksWS->getPeak(0);
      TSM_ASSERT_EQUALS("Wrong shape name", PeakShapeEllipsoid::ellipsoidShapeName(), firstPeak.getPeakShape().shapeName() );

  }







};
