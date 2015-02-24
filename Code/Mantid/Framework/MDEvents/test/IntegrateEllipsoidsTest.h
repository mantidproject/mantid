#include <cxxtest/TestSuite.h>
#include "MantidMDEvents/IntegrateEllipsoids.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
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

  void addFakeEllipsoid(const int& detectorId, const int& totalNPixels, const double tofExact, const int& nEvents, const NearestNeighbours& nn, EventWorkspace_sptr& eventWS)
  {
      EventList& el = eventWS->getOrAddEventList(detectorId - totalNPixels);
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
          EventList neighbourEventList = eventWS->getOrAddEventList(neighbourDet - totalNPixels);
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
      // Add some peaks which should correspond to real reflections (could calculate these)
      Peak* a = peaksWS->createPeakHKL(V3D(1, -5, -3));
      Peak* b = peaksWS->createPeakHKL(V3D(1, -4, -4));
      Peak* c = peaksWS->createPeakHKL(V3D(1, -3, -5));
      peaksWS->addPeak(*c);
      peaksWS->addPeak(*a);
      peaksWS->addPeak(*b);
      // Make an event workspace and add fake peak data
      auto eventWS = boost::make_shared<EventWorkspace>();
      eventWS->setInstrument(inst);
      eventWS->initialize(nPixels * nPixels /*n spectra*/, 3 /* x-size */, 3 /* y-size */);

      // Make a nn map, so that we can add counts in the vicinity of the actual peak centre.
      const int nPixelsTotal = nPixels*nPixels;
      NearestNeighbours nn(inst, buildSpectrumDetectorMapping(nPixelsTotal, nPixelsTotal+inst->getNumberDetectors() - 1));

      // Add a fake ellipsoid
      addFakeEllipsoid(c->getDetectorID(), nPixelsTotal,  c->getTOF(), 10, nn, eventWS);
      addFakeEllipsoid(a->getDetectorID(), nPixelsTotal,  a->getTOF(), 10, nn, eventWS);
      addFakeEllipsoid(b->getDetectorID(), nPixelsTotal,  b->getTOF(), 10, nn, eventWS);

      // Clean up
      delete a;
      delete b;
      delete c;

      // Return test data.
      return boost::tuple<EventWorkspace_sptr, PeaksWorkspace_sptr>(eventWS, peaksWS);
  }

  void test_init()
  {
      Mantid::MDEvents::IntegrateEllipsoids alg;
      TS_ASSERT_THROWS_NOTHING(alg.initialize());
  }

  void test_execution()
  {
      auto out = createDiffractionData();
  }







};
