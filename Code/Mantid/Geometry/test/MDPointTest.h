#ifndef MD_POINT_TEST_H
#define MD_POINT_TEST_H

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/MDGeometry/MDPoint.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/V3D.h"
#include <memory>

class MDPointTest :    public CxxTest::TestSuite
{

private:

  //TODO : replace fakes with Mocks using gmock.
  class DummyDetector : public Mantid::Geometry::Detector
  {
  public:
    DummyDetector(std::string name) : Mantid::Geometry::Detector(name, NULL) {}
    ~DummyDetector() {}
  };

  //TODO : replace fakes with Mocks using gmock.
  class DummyInstrument : public Mantid::Geometry::Instrument
  {
  public:
    DummyInstrument(std::string name) : Mantid::Geometry::Instrument(name) {}
    ~DummyInstrument() {}
  };

  //Helper constructional method.
  static std::auto_ptr<Mantid::Geometry::MDPoint> constructMDPoint()
  {
    using namespace Mantid::Geometry;
    std::vector<coordinate> vertexes;
    coordinate c;
    c.x = 1;
    c.y = 2;
    c.z = 3;
    c.t = 4;
    vertexes.push_back(c);
    IDetector_sptr detector = IDetector_sptr(new DummyDetector("dummydetector"));
    IInstrument_sptr instrument = IInstrument_sptr(new DummyInstrument("dummyinstrument"));
    return std::auto_ptr<MDPoint>(new MDPoint(1, 0.1, vertexes, detector, instrument));
  }

public:

  void testGetSignal()
  {
    using namespace Mantid::Geometry;
    std::auto_ptr<MDPoint> point = constructMDPoint();
    TSM_ASSERT_EQUALS("The signal value is not wired-up correctly", 1, point->getSignal()); 
  }

  void testGetError()
  {
    using namespace Mantid::Geometry;
    std::auto_ptr<MDPoint> point = constructMDPoint();
    TSM_ASSERT_EQUALS("The error value is not wired-up correctly", 0.1, point->getError()); 
  }

  void testGetContributingPointsThrows()
  {
    using namespace Mantid::Geometry;
    std::auto_ptr<MDPoint> point = constructMDPoint();
    TSM_ASSERT_THROWS("Attempting to fetching contributing points on a point should throw", point->getContributingPoints(), std::logic_error);
  }

  void testGetDetector()
  {
    using namespace Mantid::Geometry;
    std::auto_ptr<MDPoint> point = constructMDPoint();
    TSM_ASSERT_EQUALS("The detector getter is not wired-up correctly", "dummydetector", point->getDetector()->getName());
  }

  void testGetInstrument()
  {
    using namespace Mantid::Geometry;
    std::auto_ptr<MDPoint> point = constructMDPoint();
    TSM_ASSERT_EQUALS("The instrument getter is not wired-up correctly", "dummyinstrument", point->getInstrument()->getName());
  }

  void testGetVertexes()
  {
    using namespace Mantid::Geometry;
    std::auto_ptr<MDPoint> point = constructMDPoint();
    std::vector<coordinate> vertexes = point->getVertexes();
    TSM_ASSERT_EQUALS("A single vertex should be present.", 1, vertexes.size());
    coordinate v1 = vertexes.at(0);
    TSM_ASSERT_EQUALS("Vertex x value incorrect", 1, v1.x);
    TSM_ASSERT_EQUALS("Vertex y value incorrect", 2, v1.y);
    TSM_ASSERT_EQUALS("Vertex z value incorrect", 3, v1.z);
    TSM_ASSERT_EQUALS("Vertex t value incorrect", 4, v1.t);
  }

};
#endif