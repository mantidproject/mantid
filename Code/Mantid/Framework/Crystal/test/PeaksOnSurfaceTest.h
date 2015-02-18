#ifndef MANTID_CRYSTAL_PEAKSONSURFACETEST_H_
#define MANTID_CRYSTAL_PEAKSONSURFACETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidCrystal/PeaksOnSurface.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"

using namespace Mantid::Crystal;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;


/*-------------------------------------------------------------------------------------------------------------------------------------------------------
Functional Tests
-------------------------------------------------------------------------------------------------------------------------------------------------------*/
class PeaksOnSurfaceTest : public CxxTest::TestSuite
{
private:

    /**
  Helper function. Creates a peaksworkspace with a single peak 
  */
  PeaksWorkspace_sptr createPeaksWorkspace(const std::string coordFrame, const Mantid::Kernel::V3D& peakPosition)
  {
    PeaksWorkspace_sptr ws = WorkspaceCreationHelper::createPeaksWorkspace(1);
    auto detectorIds = ws->getInstrument()->getDetectorIDs();
    Peak& peak = ws->getPeak(0);
    peak.setDetectorID(detectorIds.front());
    Mantid::Kernel::V3D position;
    if(coordFrame == "Q (lab frame)")
    {
      peak.setQLabFrame(peakPosition,1/*set the detector distance explicitly*/);
    }
    else
    {
      throw std::runtime_error("Coordinate frame unsported in these tests."); 
    }
    return ws;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PeaksOnSurfaceTest *createSuite() { return new PeaksOnSurfaceTest(); }
  static void destroySuite( PeaksOnSurfaceTest *suite ) { delete suite; }

  void do_test_vertex_throws(const std::string& message, const std::string& vertex1, const std::string& vertex2, const std::string& vertex3, const std::string& vertex4)
  {
    PeaksOnSurface alg;
    alg.setRethrows(true);
    alg.initialize() ;
    TS_ASSERT( alg.isInitialized() ) ;
    alg.setProperty("InputWorkspace", WorkspaceCreationHelper::createPeaksWorkspace());
    alg.setPropertyValue("CoordinateFrame", "Q (lab frame)");
    alg.setPropertyValue("Vertex1", vertex1);
    alg.setPropertyValue("Vertex2", vertex2);
    alg.setPropertyValue("Vertex3", vertex3);
    alg.setPropertyValue("Vertex4", vertex4);
    alg.setPropertyValue("OutputWorkspace", "OutWS");

    TSM_ASSERT_THROWS(message, alg.execute(), std::invalid_argument&);
  }

  void test_too_few_entries()
  {
    do_test_vertex_throws("Too few for Vertex1", "0,0", "0,1,0", "1,1,0", "1,0,0");
    do_test_vertex_throws("Too few for Vertex2", "0,0,0", "0,1", "1,1,0", "1,0,0");
    do_test_vertex_throws("Too few for Vertex3", "0,0,0", "0,1,0", "1,1", "1,0,0");
    do_test_vertex_throws("Too few for Vertex4", "0,0,0", "0,1,0", "1,1,0", "1,0");
  }

  void test_too_many_entries()
  {
    do_test_vertex_throws("Too many for Vertex1", "0,0,0,0", "0,1,0", "1,1,0", "1,0,0");
    do_test_vertex_throws("Too many for Vertex2", "0,0,0", "0,1,0,0", "1,1,0", "1,0,0");
    do_test_vertex_throws("Too many for Vertex3", "0,0,0", "0,1,0", "1,1,0,0", "1,0,0");
    do_test_vertex_throws("Too many for Vertex4", "0,0,0", "0,1,0", "1,1,0", "1,0,0,0");
  }


  void test_well_formed_vertexes()
  {
    PeaksOnSurface alg;
    alg.setRethrows(true);
    alg.initialize() ;
    TS_ASSERT( alg.isInitialized() ) ;
    alg.setProperty("InputWorkspace", WorkspaceCreationHelper::createPeaksWorkspace());
    alg.setPropertyValue("CoordinateFrame", "Q (lab frame)");
    alg.setPropertyValue("Vertex1", "0,0,0");
    alg.setPropertyValue("Vertex2", "0,1,0");
    alg.setPropertyValue("Vertex3", "1,1,0");
    alg.setPropertyValue("Vertex4", "1,0,0");
    alg.setPropertyValue("OutputWorkspace", "OutWS");
    TSM_ASSERT_THROWS_NOTHING("Input Vertexes are well formed", alg.execute());
  }

  void test_vertexes_not_coplanar()
  {
    PeaksOnSurface alg;
    alg.setRethrows(true);
    alg.initialize() ;
    TS_ASSERT( alg.isInitialized() ) ;
    alg.setProperty("InputWorkspace", WorkspaceCreationHelper::createPeaksWorkspace());
    alg.setPropertyValue("CoordinateFrame", "Q (lab frame)");
    alg.setPropertyValue("Vertex1", "0,0,0");
    alg.setPropertyValue("Vertex2", "0,1,0");
    alg.setPropertyValue("Vertex3", "0.5,0.5,0.707106"); // x^2 + y^2 + z^2 == 1, but is not coplanar.
    alg.setPropertyValue("Vertex4", "1,0,0");
    alg.setPropertyValue("OutputWorkspace", "OutWS");
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument&);
  }

  void test_vertexes_not_square_sided()
  {
    PeaksOnSurface alg;
    alg.setRethrows(true);
    alg.initialize() ;
    TS_ASSERT( alg.isInitialized() ) ;
    alg.setProperty("InputWorkspace", WorkspaceCreationHelper::createPeaksWorkspace());
    alg.setPropertyValue("CoordinateFrame", "Q (lab frame)");
    alg.setPropertyValue("Vertex1", "0,0,0");
    alg.setPropertyValue("Vertex2", "0,1,0");
    alg.setPropertyValue("Vertex3", "0.5,1.2247,0"); // x^2 + y^2 + z^2 == 1, and is coplanar, but not square sided.
    alg.setPropertyValue("Vertex4", "1,0,0");
    alg.setPropertyValue("OutputWorkspace", "OutWS");
    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument&);
  }

  /*
  Define a surface at constant z = 0, as follows.

  (0,1,0)|-----------------|(1,1,0)
         |                 |
         |                 |
         |                 |
         |                 |
         |                 |
  (0,0,0)|-----------------|(1,0,0)

  Place a point just behind the plane at (0.5, 0.5, 1). It has a radius of 1 so it should intersect the plane.
  */
  void test_sphere_intersects_surface()
  {
    // Create a workspace with a point at 0.5, 0.5, 1
    auto ws = this->createPeaksWorkspace("Q (lab frame)", V3D(0.5,0.5,1));

    const std::string outName ="outWS";
    PeaksOnSurface alg;
    alg.setRethrows(true);
    alg.initialize() ;
    TS_ASSERT( alg.isInitialized() ) ;
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("CoordinateFrame", "Q (lab frame)");
    alg.setPropertyValue("Vertex1", "0,0,0");
    alg.setPropertyValue("Vertex2", "0,1,0");
    alg.setPropertyValue("Vertex3", "1,1,0"); 
    alg.setPropertyValue("Vertex4", "1,0,0");
    alg.setProperty("PeakRadius", 1.0); // Just intersects the surface.
    alg.setPropertyValue("OutputWorkspace", outName);
    alg.execute();

    ITableWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outName);

    TS_ASSERT_EQUALS(3, outWS->columnCount());
    TS_ASSERT_EQUALS("PeakIndex", outWS->getColumn(0)->name());
    TS_ASSERT_EQUALS("Intersecting", outWS->getColumn(1)->name());
    TS_ASSERT_EQUALS("Distance", outWS->getColumn(2)->name());
    TS_ASSERT_EQUALS(1, outWS->rowCount());

    TSM_ASSERT_EQUALS("Peak index should be zero", 0, outWS->cell<int>(0,  0)); 
    TSM_ASSERT_EQUALS("Peak intersect should be true", Boolean(true), outWS->cell<Boolean>(0,  1));
    TSM_ASSERT_DELTA("Wrong distance calculated", 1.0, outWS->cell<double>(0, 2), 0.0001);
  }

  
  /*
  Define a surface at constant z = 0, as follows.

  (0,1,0)|-----------------|(1,1,0)
         |                 |
         |                 |
         |                 |
         |                 |
         |                 |
  (0,0,0)|-----------------|(1,0,0)

  Place a point just behind the plane at (0.5, 0.5, 1). It has a radius of 0.999 so it should just MISS intersecting the plane.
  */
  void test_sphere_doesnt_intersect_plane_or_surface()
  {
    // Create a workspace with a point at 0.5, 0.5, 1
    auto ws = this->createPeaksWorkspace("Q (lab frame)", V3D(0.5,0.5,1));

    const std::string outName ="outWS";
    PeaksOnSurface alg;
    alg.setRethrows(true);
    alg.initialize() ;
    TS_ASSERT( alg.isInitialized() ) ;
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("CoordinateFrame", "Q (lab frame)");
    alg.setPropertyValue("Vertex1", "0,0,0");
    alg.setPropertyValue("Vertex2", "0,1,0");
    alg.setPropertyValue("Vertex3", "1,1,0"); 
    alg.setPropertyValue("Vertex4", "1,0,0");
    alg.setProperty("PeakRadius", 0.999); // Just miss intersecting the surface.
    alg.setPropertyValue("OutputWorkspace", outName);
    alg.execute();

    ITableWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outName);

    TSM_ASSERT_EQUALS("Peak index should be zero", 0, outWS->cell<int>(0,  0)); 
    TSM_ASSERT_EQUALS("Peak intersect should be false", Boolean(false), outWS->cell<Boolean>(0,  1));
  }


    /*
  Define a surface at constant z = 0, as follows.

  (0,1,0)|-----------------|(1,1,0)
         |                 |
         |                 |
         |                 |
         |                 |
         |                 |
  (0,0,0)|-----------------|(1,0,0)           (2,0,0) Peak Here

  Place a point on the plane exactly at 2,0,0, but has a radius of 0.999, and is therefore outside the surface boundaries.
  */
  void test_peak_on_plane_but_outside_surface()
  {
    // Create a workspace with a point at 2.0, 0.0, 0.0
    auto ws = this->createPeaksWorkspace("Q (lab frame)", V3D(2.0, 0.0, 1e-9));

    const std::string outName ="outWS";
    PeaksOnSurface alg;
    alg.setRethrows(true);
    alg.initialize() ;
    TS_ASSERT( alg.isInitialized() ) ;
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("CoordinateFrame", "Q (lab frame)");
    alg.setPropertyValue("Vertex1", "0,0,0");
    alg.setPropertyValue("Vertex2", "0,1,0");
    alg.setPropertyValue("Vertex3", "1,1,0"); 
    alg.setPropertyValue("Vertex4", "1,0,0");
    alg.setProperty("PeakRadius", 0.9); // Just miss intersecting the surface.
    alg.setPropertyValue("OutputWorkspace", outName);
    alg.execute();

    ITableWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outName);

    TSM_ASSERT_EQUALS("Peak index should be zero", 0, outWS->cell<int>(0,  0)); 
    TSM_ASSERT_EQUALS("Peak intersect should be false", Boolean(false), outWS->cell<Boolean>(0,  1));
    TSM_ASSERT_EQUALS("Wrong distance calculated", 0.0, outWS->cell<double>(0, 2));
  }

      /*
  Define a surface at constant z = 0, as follows.

  (0,1,0)|-----------------|(1,1,0)
         |                 |
         |                 |
         |                 |
         |                 |
         |                 |
  (0,0,0)|-----------------|(1,0,0)           (2,0,0) Peak Here

  Place a point on the plane exactly at 2,0,0, but has a radius of 1, and is therefore does cross the surface boundaries.
  */
  void test_peak_on_plane_and_crosses_surface()
  {
    // Create a workspace with a point at 2.0, 0.0, 0.0
    auto ws = this->createPeaksWorkspace("Q (lab frame)", V3D(2.0, 0.0, 1e-9));

    const std::string outName ="outWS";
    PeaksOnSurface alg;
    alg.setRethrows(true);
    alg.initialize() ;
    TS_ASSERT( alg.isInitialized() ) ;
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("CoordinateFrame", "Q (lab frame)");
    alg.setPropertyValue("Vertex1", "0,0,0");
    alg.setPropertyValue("Vertex2", "0,1,0");
    alg.setPropertyValue("Vertex3", "1,1,0"); 
    alg.setPropertyValue("Vertex4", "1,0,0");
    alg.setProperty("PeakRadius", 1.0); // Just miss intersecting the surface.
    alg.setPropertyValue("OutputWorkspace", outName);
    alg.execute();

    ITableWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outName);

    TSM_ASSERT_EQUALS("Peak index should be zero", 0, outWS->cell<int>(0,  0)); 
    TSM_ASSERT_EQUALS("Peak intersect should be false", Boolean(true), outWS->cell<Boolean>(0,  1));
    TSM_ASSERT_EQUALS("Wrong distance calculated", 0.0, outWS->cell<double>(0, 2));
  }

  void test_line_intersects_sphere_facility()
  {
    V3D peakCenter(0,0,0);
    const double peakRadius = 1;
    const double delta = 0.01;
    const V3D lineStart(-1, 1, 0);
    const V3D lineEnd(1, 1, 0);
    const V3D line = lineEnd - lineStart; //Defines a line running horzontal along x at y = 1 and z = 0, between x = -1 and 1

    TSM_ASSERT("Should just intersect sphere", lineIntersectsSphere(line, lineStart, peakCenter, peakRadius));

    TSM_ASSERT("Should just skim but not intersect the sphere", !lineIntersectsSphere(line, lineStart, peakCenter, peakRadius - delta));

    TSM_ASSERT("Should fully intersect sphere", lineIntersectsSphere(line, lineStart, peakCenter, peakRadius + delta));

    // Now move the peak center to give a scenario, where the line segment would not intersect the sphere, but the the infinite line would.
    peakCenter = V3D(2, 1, 0);
    TSM_ASSERT("Line segment does NOT intersect sphere, but infinite line does", !lineIntersectsSphere(line, lineStart, peakCenter, peakRadius - delta));

    TSM_ASSERT("Line segment does Just intersect sphere", lineIntersectsSphere(line, lineStart, peakCenter, peakRadius + delta));
  }
};

/*-------------------------------------------------------------------------------------------------------------------------------------------------------
Perfomance Tests
-------------------------------------------------------------------------------------------------------------------------------------------------------*/
class PeaksOnSurfaceTestPerformance : public CxxTest::TestSuite
{

private:

  Mantid::API::IPeaksWorkspace_sptr inputWS;

public:

  static PeaksOnSurfaceTestPerformance *createSuite() { return new PeaksOnSurfaceTestPerformance(); }
  static void destroySuite( PeaksOnSurfaceTestPerformance *suite ) { delete suite; }

  PeaksOnSurfaceTestPerformance()
  {
    int numPeaks = 4000;
    inputWS = boost::make_shared<PeaksWorkspace>();
    Mantid::Geometry::Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, 200);
    inputWS->setInstrument(inst);

    for (int i = 0; i < numPeaks; ++i)
    {
      Peak peak(inst, i, i+-0.5);
      inputWS->addPeak(peak);
    }
  }

  void test_performance()
  {
    const std::string outName = "OutPerfWS";

    PeaksOnSurface alg;
    alg.setRethrows(true);
    alg.initialize() ;
    TS_ASSERT( alg.isInitialized() ) ;
    alg.setProperty("InputWorkspace", inputWS);
    alg.setPropertyValue("CoordinateFrame", "Detector space");
    alg.setPropertyValue("Vertex1", "0.5, -1, 1");
    alg.setPropertyValue("Vertex2", "0.5, 1, 1");
    alg.setPropertyValue("Vertex3", "1, 1, 1");
    alg.setPropertyValue("Vertex4", "1, -1, 1");
    alg.setPropertyValue("OutputWorkspace", outName);
    alg.setProperty("PeakRadius", 0.4); 
    alg.execute();

    Mantid::API::ITableWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(outName);

    TS_ASSERT_EQUALS(3, outWS->columnCount());
    TS_ASSERT_EQUALS(inputWS->rowCount(), outWS->rowCount());
  }
};

#endif /* MANTID_CRYSTAL_PEAKSONSURFACETEST_H_ */
