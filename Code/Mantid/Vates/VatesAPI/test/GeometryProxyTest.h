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

  //Helper methods purely to keep dimension ids consistent.
  static std::string getXDimId(){return "qx";}
  static std::string getYDimId(){return "qy";}
  static std::string getZDimId(){return "qz";}
  static std::string getTDimId(){return "en";}

  //Helper type to generate unique numbers from i, j, k, combinations. 
  //Assume arguments are between 0 and 9. Crude, but sufficient for these test scenarios
  struct uniqueArgumentCombination
  {
    double operator()(int i, int j, int k, int t)
    {
      return i * 1000 + j * 100 + k * 10 + t;
    }
  };


  //MDDimension stores dimensions as their concrete type, so cannot Mock-out IMDDimensions, which is the preferred approach. Have to subclass MDDimensions
  //and override the specific method that the client code uses. This is not ideal!
  class DLLExport FakeDimension: public Mantid::Geometry::MDDimension
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
    size_t getNBins() const
    {
      return m_nbins;
    }
  };

  class DLLExport GeometryPolicy
  {
  public:
    typedef boost::shared_ptr<FakeDimension> Dimension_sptr_type;

    GeometryPolicy(int i, int j, int k, int t) : m_i(i), m_j(j), m_k(k), m_t(t) 
    {
    }

    boost::shared_ptr<FakeDimension> getXDimension() const
    {
      using namespace Mantid::Geometry;
      return boost::shared_ptr<FakeDimension>(new FakeDimension(getXDimId(), 0, 1, m_i));
    }
    boost::shared_ptr<FakeDimension> getYDimension() const
    {
      using namespace Mantid::Geometry;
      return boost::shared_ptr<FakeDimension>(new FakeDimension(getYDimId(), 0, 1, m_j));
    }
    boost::shared_ptr<FakeDimension> getZDimension() const
    {
      using namespace Mantid::Geometry;
      return boost::shared_ptr<FakeDimension>(new FakeDimension(getZDimId(), 0, 1, m_k));
    }
    boost::shared_ptr<FakeDimension> getTDimension() const
    {
      using namespace Mantid::Geometry;
      return boost::shared_ptr<FakeDimension>(new FakeDimension(getTDimId(), 0, 1, m_t));
    }

  private:
    const int m_i;
    const int m_j;
    const int m_k;
    const int m_t;
  };

  /// Image Policy utilises compile-time polymorphism in vtkDataSetFactories
  /// for testing purposes. Otherwise too unwieldy to generate MDImage from scratch.
  class DLLExport ImagePolicy
  {

  public:
    /// Embedded type information
    typedef GeometryPolicy GeometryType;

    ImagePolicy(int i, int j, int k, int t): m_geometry(i, j, k, t)
    {
    }

    GeometryType* getGeometry()
    {
      return &m_geometry;
    }

    /// Get the MDImagePoint
    Mantid::MDDataObjects::MD_image_point getPoint(int i, int j, int k, int t) const
    {
      Mantid::MDDataObjects::MD_image_point point;
      uniqueArgumentCombination unique;
      point.s = unique(i, j, k, t);
      return point;
    }

    private:
    GeometryPolicy m_geometry;
  };

  
  typedef boost::shared_ptr<Mantid::VATES::GeometryProxy<ImagePolicy> > GeometryProxy_sptr;
  typedef boost::shared_ptr<ImagePolicy> ImagePolicy_sptr;
  typedef boost::function<Mantid::MDDataObjects::MD_image_point(int, int, int, int)> GetPoint_fctn;

  static GeometryProxy_sptr constructGeometryProxy(
    ImagePolicy_sptr image, 
    Mantid::VATES::Dimension_sptr xDim, 
    Mantid::VATES::Dimension_sptr yDim, 
    Mantid::VATES::Dimension_sptr zDim, 
    Mantid::VATES::Dimension_sptr tDim)
  {
    using namespace Mantid::VATES;
    return GeometryProxy_sptr(GeometryProxy<ImagePolicy>::New(image, xDim, yDim, zDim, tDim));
  }

public:

  // Just to be clear here. GeometryProxy (domain type) is handed GeometryPolicy (test type), which is a substitute for MDGeometry.
  // Likewise, ImagePolicy is a substitute for MDImage

  void testNormalDimensionMappings()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getXDimId(), 1, 2, 3)); //Mirrors geometry x dimension mapping
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getYDimId(), 1, 2, 3)); //Mirrors geometry y dimension mapping
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getZDimId(), 1, 2, 3)); //Mirrors geometry z dimension mapping
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getTDimId(), 1, 2, 3)); //Mirrors geometry t dimension mapping
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    TS_ASSERT_EQUALS(getXDimId(), proxy->getXDimension()->getDimensionId());
    TS_ASSERT_EQUALS(getYDimId(), proxy->getYDimension()->getDimensionId());
    TS_ASSERT_EQUALS(getZDimId(), proxy->getZDimension()->getDimensionId()); 
    TS_ASSERT_EQUALS(getTDimId(), proxy->getTDimension()->getDimensionId()); 
    
  }

  void testRemappedDimensions()
  {
    using namespace Mantid::VATES;
    //  We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getTDimId(), 1, 2, 3)); //New alignment
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getZDimId(), 1, 2, 3)); //New alignment
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getYDimId(), 1, 2, 3)); //New alignment
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getXDimId(), 1, 2, 3)); //Mirrors geometry t dimension mapping

    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    TS_ASSERT_EQUALS(getTDimId(), proxy->getXDimension()->getDimensionId());
    TS_ASSERT_EQUALS(getZDimId(), proxy->getYDimension()->getDimensionId());
    TS_ASSERT_EQUALS(getYDimId(), proxy->getZDimension()->getDimensionId());
    TS_ASSERT_EQUALS(getXDimId(), proxy->getTDimension()->getDimensionId());
  }

  void testRemapPoints_xyzt()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getXDimId(), 1, 2, 3)); //Mirrors geometry x dimension mapping
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getYDimId(), 1, 2, 3)); //Mirrors geometry y dimension mapping
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getZDimId(), 1, 2, 3)); //Mirrors geometry z dimension mapping
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getTDimId(), 1, 2, 3)); //Mirrors geometry t dimension mapping
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for xyzt schenario", unique(1, 2, 3, 4), function(1, 2, 3, 4).s);
  }

  void testRemapPoints_xzyt()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getXDimId(), 1, 2, 3)); // x -> x
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getZDimId(), 1, 2, 3)); // y -> y
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getYDimId(), 1, 2, 3)); // z -> z
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getTDimId(), 1, 2, 3)); // t -> t
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for xzyt schenario", unique(1, 3, 2, 4), function(1, 2, 3, 4).s);
  }


  void testRemapPoints_yxzt()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getYDimId(), 1, 2, 3)); // x -> y (so x becomes 2nd argument)
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getXDimId(), 1, 2, 3)); // y -> x (so y becomes 1st argument)
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getZDimId(), 1, 2, 3)); // z -> z (so z becomes 3rd argument)
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getTDimId(), 1, 2, 3)); // t -> t (so t becomes 4th argument)
    //hence yxzt
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for yxzt schenario", unique(2, 1, 3, 4), function(1, 2, 3, 4).s);
  }

  void testRemapPoints_yzxt()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getZDimId(), 1, 2, 3)); // x -> z (so becomes 3nd argument)
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getXDimId(), 1, 2, 3)); // y -> x (so becomes 1st argument)
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getYDimId(), 1, 2, 3)); // z -> y (so becomes 2nd argument)
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getTDimId(), 1, 2, 3)); // t -> t (so becomes 4th argument)
    //hence yzxt
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for yzxt schenario", unique(2, 3, 1, 4), function(1, 2, 3, 4).s);
  }

  void testRemapPoints_zxyt()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getYDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getZDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getXDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getTDimId(), 1, 2, 3)); 
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim); //zxyt 

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for zxyt schenario", unique(3, 1, 2, 4), function(1, 2, 3, 4).s);
  }

  void testRemapPoints_txyz()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getYDimId(), 1, 2, 3)); // x -> y
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getZDimId(), 1, 2, 3)); // y -> z
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getTDimId(), 1, 2, 3)); // z -> t
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getXDimId(), 1, 2, 3)); // t -> x
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim); //txyz

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for txyz schenario", unique(4, 1, 2, 3), function(1, 2, 3, 4).s);
  }

  void testRemapPoints_txzy()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getYDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getTDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getZDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getXDimId(), 1, 2, 3)); 
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for txzy schenario", unique(4, 1, 3, 2), function(1, 2, 3, 4).s);
  }


  void testRemapPoints_tyxz()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getZDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getYDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getTDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getXDimId(), 1, 2, 3)); 
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for tyxz schenario", unique(4, 2, 1, 3), function(1, 2, 3, 4).s);
  }

  void testRemapPoints_tyzx()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getTDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getYDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getZDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getXDimId(), 1, 2, 3)); 
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for tyzx schenario", unique(4, 2, 3, 1), function(1, 2, 3, 4).s);
  }

    void testRemapPoints_tzxy()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getZDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getTDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getYDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getXDimId(), 1, 2, 3)); 
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for tzxy schenario", unique(4, 3, 1, 2), function(1, 2, 3, 4).s);
  }

        void testRemapPoints_tzyx()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getTDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getZDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getYDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getXDimId(), 1, 2, 3)); 
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for tzyx schenario", unique(4, 3, 2, 1), function(1, 2, 3, 4).s);
  }

  void testRemapPoints_xtyz()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getXDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getZDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getTDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getYDimId(), 1, 2, 3)); 
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for xtyz schenario", unique(1, 4, 2, 3), function(1, 2, 3, 4).s);
  }

  void testRemapPoints_xtzy()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getXDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getTDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getZDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getYDimId(), 1, 2, 3)); 
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for xtzy schenario", unique(1, 4, 3, 2), function(1, 2, 3, 4).s);
  }

  void testRemapPoints_ytxz()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getZDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getXDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getTDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getYDimId(), 1, 2, 3)); 
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for ytxz schenario", unique(2, 4, 1, 3), function(1, 2, 3, 4).s);
  }

  void testRemapPoints_ytzx()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getTDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getXDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getZDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getYDimId(), 1, 2, 3)); 
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for ytzx schenario", unique(2, 4, 3, 1), function(1, 2, 3, 4).s);
  }

    void testRemapPoints_ztxy()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getZDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getTDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getXDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getYDimId(), 1, 2, 3)); 
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for ztxy schenario", unique(3, 4, 1, 2), function(1, 2, 3, 4).s);
  }

  void testRemapPoints_ztyx()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getTDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getZDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getXDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getYDimId(), 1, 2, 3)); 
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for ztyx  schenario", unique(3, 4, 2, 1), function(1, 2, 3, 4).s);
  }

  void testRemapPoints_xytz()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getXDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getYDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getTDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getZDimId(), 1, 2, 3)); 
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for xytz schenario", unique(1, 2, 4, 3), function(1, 2, 3, 4).s);
  }

    void testRemapPoints_xzty()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getXDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getTDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getYDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getZDimId(), 1, 2, 3)); 
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for xzty schenario", unique(1, 3, 4, 2), function(1, 2, 3, 4).s);
  }

   void testRemapPoints_yxtz()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getYDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getXDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getTDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getZDimId(), 1, 2, 3)); 
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for yxtz schenario", unique(2, 1, 4, 3), function(1, 2, 3, 4).s);
  }

  void testRemapPoints_yztx()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getTDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getXDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getYDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getZDimId(), 1, 2, 3)); 
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for yztx schenario", unique(2, 3, 4, 1), function(1, 2, 3, 4).s);
  }

  void testRemapPoints_zxty()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getYDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getTDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getXDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getZDimId(), 1, 2, 3)); 
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for zxty schenario", unique(3, 1, 4, 2), function(1, 2, 3, 4).s);
  }

  void testRemapPoints_zytx()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension(getTDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getYDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getXDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getZDimId(), 1, 2, 3)); 
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    GetPoint_fctn function = proxy->getMappedPointFunction();

    uniqueArgumentCombination unique;
    TSM_ASSERT_EQUALS("Rebinding has not been done correctly for zytx schenario", unique(3, 2, 4, 1), function(1, 2, 3, 4).s);
  }

  void testRemapPoints_throws()
  {
    using namespace Mantid::VATES;
    //We are going to use new mappings to describe how dimensions are actually to be used.
    Mantid::VATES::Dimension_sptr xDim(new FakeDimension("--", 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr yDim(new FakeDimension(getYDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr zDim(new FakeDimension(getXDimId(), 1, 2, 3)); 
    Mantid::VATES::Dimension_sptr tDim(new FakeDimension(getZDimId(), 1, 2, 3)); 
    
    ImagePolicy_sptr imagePolicy = ImagePolicy_sptr(new ImagePolicy(1, 2, 3, 4));
    GeometryProxy_sptr proxy = constructGeometryProxy(imagePolicy, xDim, yDim, zDim, tDim);

    TSM_ASSERT_THROWS("Cannot rebind, so should throw, exception.", proxy->getMappedPointFunction(), std::runtime_error);
  }


};
#endif
