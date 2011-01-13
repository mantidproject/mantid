#include "GeometryWidget.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "boost/shared_ptr.hpp"
#include <map>

GeometryWidget::GeometryWidget(Mantid::Geometry::MDGeometry const * const geometry)
{
//  using namespace Mantid::Geometry;
//  using Mantid::Geometry::DimensionVec;
//  typedef boost::shared_ptr<IMDDimension> spDimension;
//
//  spDimension xDimension = geometry->getXDimension();
//  spDimension yDimension = geometry->getYDimension();
//  spDimension zDimension = geometry->getZDimension();
//  spDimension tDimension = geometry->getTDimension();
//
//  std::map<spDimension, bool> occupancyMap;
//
//  DimensionVec vecDimensions = geometry->getDimensions();
//  DimensionVecIterator it = vecDimensions.begin();
//  for(;it != vecDimensions.end(); ++it)
//  {
//    boost::shared_ptr<IMDDimension> spDimension = *it;
//    //Create an integrated dimension;
//  }
}

GeometryWidget::~GeometryWidget()
{
}
