#ifndef MD_CELL_TEST_H
#define MD_CELL_TEST_H

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/MDGeometry/MDPoint.h"
#include "MantidGeometry/MDGeometry/MDCell.h"

class MDCellTest :    public CxxTest::TestSuite
{

private:
  //TODO: replace with gmock versions in future.
  class FakeMDPoint : public  Mantid::Geometry::MDPoint
  {
  public:
    
    double getSignal() const
    {
      return 1;
    }

    double getError() const
    {
      return 0.1;
    }
  };

    //Helper constructional method.
  static std::auto_ptr<Mantid::Geometry::MDCell> constructMDCell()
  {
    using namespace Mantid::Geometry;
    std::vector<coordinate> vertexes;
    coordinate c;
    c.x = 4;
    c.y = 3;
    c.z = 2;
    c.t = 1;
    vertexes.push_back(c);

    std::vector<boost::shared_ptr<MDPoint> > points;
    points.push_back(boost::shared_ptr<MDPoint>(new FakeMDPoint));
    points.push_back(boost::shared_ptr<MDPoint>(new FakeMDPoint));

    return std::auto_ptr<MDCell>(new MDCell(points, vertexes));
  }

public:

  void testGetSignal()
  {
    using namespace Mantid::Geometry;
    std::auto_ptr<MDCell> cell = constructMDCell();
    TSM_ASSERT_EQUALS("The signal value is not wired-up correctly", 2, cell->getSignal()); 
  }

  void testGetError()
  {
    using namespace Mantid::Geometry;
    std::auto_ptr<MDCell> cell = constructMDCell();
    TSM_ASSERT_EQUALS("The error value is not wired-up correctly", 0.2, cell->getError()); 
  }

  void testGetContributingPoints()
  {
    using namespace Mantid::Geometry;
    std::auto_ptr<MDCell> point = constructMDCell();
    std::vector<boost::shared_ptr<MDPoint> > contributingPoints = point->getContributingPoints();
    TSM_ASSERT_EQUALS("Wrong number of contributing points returned", 2, contributingPoints.size());
    TSM_ASSERT("First contributing point is null", NULL != contributingPoints.at(0).get());
    TSM_ASSERT("Second contributing point is null", NULL != contributingPoints.at(1).get());
  }


  void testGetVertexes()
  {
    using namespace Mantid::Geometry;
    std::auto_ptr<MDCell> cell = constructMDCell();
    std::vector<coordinate> vertexes = cell->getVertexes();
    TSM_ASSERT_EQUALS("A single vertex should be present.", 1, vertexes.size());
    coordinate v1 = vertexes.at(0);
    TSM_ASSERT_EQUALS("Vertex x value incorrect", 4, v1.x);
    TSM_ASSERT_EQUALS("Vertex y value incorrect", 3, v1.y);
    TSM_ASSERT_EQUALS("Vertex z value incorrect", 2, v1.z);
    TSM_ASSERT_EQUALS("Vertex t value incorrect", 1, v1.t);
  }


};
#endif