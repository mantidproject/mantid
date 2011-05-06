
#ifndef VTKDATASETFACTORYTEST_H_
#define VTKDATASETFACTORYTEST_H_

#include "MantidMDAlgorithms/Load_MDWorkspace.h"
#include "MDDataObjects/MDWorkspace.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>


class vtkDataSetFactoryTest
{

protected:

  /// Geometry Policy utilises compile-time polymorphism in vtkDataSetFactories
  /// for testing purposes. Otherwise too unwieldy to generate MDGeometry from scratch.
  class GeometryPolicy
  {
  public:

    GeometryPolicy(int i, int j, int k, int t) : m_i(i), m_j(j), m_k(k), m_t(t)
    {
    }

    boost::shared_ptr<Mantid::Geometry::IMDDimension> getXDimension() const
    {
      using namespace Mantid::Geometry;
      MDDimension* dimension = new MDDimension("qx");
      dimension->setRange(0, 1, m_i);
      return boost::shared_ptr<IMDDimension>(dimension);
    }
    boost::shared_ptr<Mantid::Geometry::IMDDimension> getYDimension() const
    {
      using namespace Mantid::Geometry;
      MDDimension* dimension = new MDDimension("qy");
      dimension->setRange(0, 1, m_j);
      return boost::shared_ptr<IMDDimension>(dimension);
    }
    boost::shared_ptr<Mantid::Geometry::IMDDimension> getZDimension() const
    {
      using namespace Mantid::Geometry;
      MDDimension* dimension = new MDDimension("qz");
      dimension->setRange(0, 1, m_k);
      return boost::shared_ptr<IMDDimension>(dimension);
    }
    boost::shared_ptr<Mantid::Geometry::IMDDimension> getTDimension() const
    {
      using namespace Mantid::Geometry;
      MDDimension* dimension = new MDDimension("t");
      dimension->setRange(0, 1, m_t);
      return boost::shared_ptr<IMDDimension>(dimension);
    }

  private:
    const int m_i;
    const int m_j;
    const int m_k;
    const int m_t;
  };

  /// Image Policy utilises compile-time polymorphism in vtkDataSetFactories
  /// for testing purposes. Otherwise too unwieldy to generate MDImage from scratch.
  class ImagePolicy
  {


  public:
    /// Embedded type information
    typedef GeometryPolicy GeometryType;
    /// Get the Geometry

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
      point.s = i;
      return point;
    }

    private:
    GeometryPolicy m_geometry;
  };

};

#endif
