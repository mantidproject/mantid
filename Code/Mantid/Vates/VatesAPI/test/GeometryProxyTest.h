#ifndef GEOMETRY_PROXY_TEST_H_
#define GEOMETRY_PROXY_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidVatesAPI/GeometryProxy.h"
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

class GeometryProxyTest: public CxxTest::TestSuite
{
private:

  typedef boost::shared_ptr<Mantid::Geometry::MDDimension> MDDimension_sptr;
  typedef std::vector<MDDimension_sptr> MDDimension_vec;

  //MDDimension stores dimensions as their concrete type, so cannot Mock-out IMDDimensions, which is the preferred approach. Have to subclass MDDimensions
  //and override the specific method that the client code uses. This is not ideal!
  class FakeDimension: public Mantid::Geometry::MDDimension
  {
  private:

    double m_min;
    double m_max;
    unsigned int m_nbins;
  public:
    FakeDimension(std::string id, double min, double max, unsigned int nbins) :
      Mantid::Geometry::MDDimension(id), m_min(min), m_max(max), m_nbins(nbins)
    {
    }
    double getMaximum() const
    {
      return m_max;
    }
    double getMinimum() const
    {
      return m_min;
    }
    unsigned int getNBins() const
    {
      return m_nbins;
    }
  };

  class FakeGeometry: public Mantid::Geometry::MDGeometry
  {
  public:
    FakeGeometry(Mantid::Geometry::MDGeometryBasis& basis, MDDimension_vec dimensions) :
      Mantid::Geometry::MDGeometry(basis)

    {
      theDimension = dimensions;
    }
  };

  /// Helper method. The bulk of this code is required to create a geometry!
  static Mantid::VATES::GeometryProxy* constructGeometryProxy(Mantid::VATES::Dimension_sptr xDim,
      Mantid::VATES::Dimension_sptr yDim, Mantid::VATES::Dimension_sptr zDim,
      Mantid::VATES::Dimension_sptr tDim)
  {
    using namespace Mantid::Geometry;

    //Can't construct a geometry without basis dimensions first, so make those:
    std::set<MDBasisDimension> basisDimensions;
    basisDimensions.insert(MDBasisDimension("q1", true, 0));
    basisDimensions.insert(MDBasisDimension("q2", true, 1));
    basisDimensions.insert(MDBasisDimension("q3", true, 2));
    basisDimensions.insert(MDBasisDimension("u3", false, 3));

    //Now we need dimensions, but need derived new type (see type description for explanation).
    MDDimension_vec dimensions;
    dimensions.push_back(MDDimension_sptr(new FakeDimension("q1", 1, 2, 3)));
    dimensions.push_back(MDDimension_sptr(new FakeDimension("q2", 1, 2, 3)));
    dimensions.push_back(MDDimension_sptr(new FakeDimension("q3", 1, 2, 3)));
    dimensions.push_back(MDDimension_sptr(new FakeDimension("u3", 1, 2, 3)));

    UnitCell cell; // Not used as far as I can tell
    MDGeometryBasis basis(basisDimensions, cell);

    //Finally we have the geometry (all above code required just to make a geometry).
    FakeGeometry* pGeometry = new FakeGeometry(basis, dimensions);

    using Mantid::VATES::GeometryProxy;
    return GeometryProxy::New(pGeometry, xDim, yDim, zDim, tDim);
  }

public:

  void testNormalMappings()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension("q1", 1, 2, 3)); //Mirrors geometry x dimension mapping
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension("q2", 1, 2, 3)); //Mirrors geometry y dimension mapping
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension("q3", 1, 2, 3)); //Mirrors geometry z dimension mapping
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension("u3", 1, 2, 3)); //Mirrors geometry t dimension mapping

    GeometryProxy* proxy = constructGeometryProxy(xDim, yDim, zDim, tDim);

    TS_ASSERT_EQUALS("q1", proxy->getXDimension()->getDimensionId());
    TS_ASSERT_EQUALS("q2", proxy->getYDimension()->getDimensionId());
    TS_ASSERT_EQUALS("q3", proxy->getZDimension()->getDimensionId());
  }

  void testRemapped()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension("q3", 1, 2, 3)); //New alignment
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension("q2", 1, 2, 3)); //New alignment
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension("q1", 1, 2, 3)); //New alignment
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension("u3", 1, 2, 3)); //Mirrors geometry t dimension mapping

    GeometryProxy* proxy = constructGeometryProxy(xDim, yDim, zDim, tDim);

    TS_ASSERT_EQUALS("q3", proxy->getXDimension()->getDimensionId());
    TS_ASSERT_EQUALS("q2", proxy->getYDimension()->getDimensionId());
    TS_ASSERT_EQUALS("q1", proxy->getZDimension()->getDimensionId());
  }

  void testIsXDimension()
  {
    using namespace Mantid::VATES;
    TS_ASSERT(isQxDimension(Dimension_sptr(new FakeDimension("q1", 1, 2, 3))));
    TS_ASSERT(isQxDimension(Dimension_sptr(new FakeDimension("q1", 1, 2, 3))));
  }

  void testIsNotXDimension()
  {
    using namespace Mantid::VATES;
    TS_ASSERT(!isQxDimension(Dimension_sptr(new FakeDimension("-", 1, 2, 3))));
  }

  void testIsYDimension()
  {
    using namespace Mantid::VATES;
    TS_ASSERT(isQyDimension(Dimension_sptr(new FakeDimension("q2", 1, 2, 3))));
    TS_ASSERT(isQyDimension(Dimension_sptr(new FakeDimension("qy", 1, 2, 3))));
  }

  void testIsNotYDimension()
  {
    using namespace Mantid::VATES;
    TS_ASSERT(!isQyDimension(Dimension_sptr(new FakeDimension("-", 1, 2, 3))));
  }

  void testIsZDimension()
  {
    using namespace Mantid::VATES;
    TS_ASSERT(isQzDimension(Dimension_sptr(new FakeDimension("q3", 1, 2, 3))));
    TS_ASSERT(isQzDimension(Dimension_sptr(new FakeDimension("qz", 1, 2, 3))));
  }

  void testIsNotZDimension()
  {
    using namespace Mantid::VATES;
    TS_ASSERT(!isQzDimension(Dimension_sptr(new FakeDimension("-", 1, 2, 3))));
  }

};
#endif
