#include "GeometryWidget.h"
#include "DimensionWidget.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include "boost/shared_ptr.hpp"
#include <map>
#include <functional>
#include <algorithm>


using Mantid::Geometry::IMDDimension;

//Comparitor Functor
struct DimensionCompare : public std::binary_function<boost::shared_ptr<IMDDimension>, boost::shared_ptr<IMDDimension>, bool>
{
  bool operator()(boost::shared_ptr<IMDDimension> a, boost::shared_ptr<IMDDimension> b)
  {
    return a->getDimensionId() == b->getDimensionId();
  }
};

GeometryWidget::GeometryWidget(Mantid::Geometry::MDGeometry const * const geometry)
{
  constructWidget(geometry);
}

//Alternative constructor: TODO: remove.
GeometryWidget::GeometryWidget(std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> > nonIntegratedVector)
{
  QGridLayout* layout = new QGridLayout;

  //Create widget to display/control the aligned x-dimension
  m_xDimensionWidget = new DimensionWidget(this, "x Dimension", 0, nonIntegratedVector);
  layout->addWidget(m_xDimensionWidget, 0, 0);

  //Create widget to display/control the aligned y-dimension
  m_yDimensionWidget = new DimensionWidget(this, "y Dimension", 1, nonIntegratedVector);
  layout->addWidget(m_yDimensionWidget, 1, 0);

  //Create widget to display/control the aligned z-dimension
  m_zDimensionWidget = new DimensionWidget(this, "z Dimension", 2, nonIntegratedVector);
  layout->addWidget(m_zDimensionWidget, 2, 0);

  //Create widget to display/control the aligned t-dimension
  m_tDimensionWidget = new DimensionWidget(this, "t Dimension", 3, nonIntegratedVector);
  layout->addWidget(m_tDimensionWidget, 3, 0);

  this->setLayout(layout);
}

void GeometryWidget::constructWidget(Mantid::Geometry::MDGeometry const * const geometry)
{
  using namespace Mantid::Geometry;
  using Mantid::Geometry::DimensionVec;
  typedef boost::shared_ptr<IMDDimension> spDimension;

  spDimension xDimension = geometry->getXDimension();
  spDimension yDimension = geometry->getYDimension();
  spDimension zDimension = geometry->getZDimension();
  spDimension tDimension = geometry->getTDimension();

  std::vector<spDimension> nonIntegratedVector;
  nonIntegratedVector.push_back(xDimension);
  nonIntegratedVector.push_back(yDimension);
  nonIntegratedVector.push_back(zDimension);
  nonIntegratedVector.push_back(tDimension);

  QGridLayout* layout = new QGridLayout;

  //Create widget to display/control the aligned x-dimension
  m_xDimensionWidget = new DimensionWidget(this, "x Dimension", 0, nonIntegratedVector);
  layout->addWidget(m_xDimensionWidget, 0, 0);

  //Create widget to display/control the aligned y-dimension
  m_yDimensionWidget = new DimensionWidget(this, "y Dimension", 1, nonIntegratedVector);
  layout->addWidget(m_yDimensionWidget, 1, 0);

  //Create widget to display/control the aligned z-dimension
  m_zDimensionWidget = new DimensionWidget(this, "z Dimension", 2, nonIntegratedVector);
  layout->addWidget(m_zDimensionWidget, 2, 0);

  //Create widget to display/control the aligned t-dimension
  m_tDimensionWidget = new DimensionWidget(this, "t Dimension", 3, nonIntegratedVector);
  layout->addWidget(m_tDimensionWidget, 3, 0);

  this->setLayout(layout);
}

std::string GeometryWidget::getXDimension() const
{
  //Get the selected alignment for the xdimension.
  return m_xDimensionWidget->getDimension()->toXMLString();
}

GeometryWidget::~GeometryWidget()
{
}

void GeometryWidget::childAppliedNewDimensionSelection(const unsigned int oldDimensionIndex,
    boost::shared_ptr<IMDDimension> newDimension, DimensionWidget* pDimensionWidget)
{
  //Updates all child guis with overwrite dimension.
  using namespace Mantid::Geometry;

  //Comparitor
  DimensionCompare areEqual;

  //The new Dimension is overwriting the dimension on this widget.
  //Assign the old widget the old dimension from the calling widget.

  if (areEqual(newDimension, m_xDimensionWidget->getDimension()))
  {
    if (pDimensionWidget != m_xDimensionWidget)
    {
      //Update the xDimensionWidget only.
      m_xDimensionWidget->populateWidget(oldDimensionIndex);
    }
  }

  if (areEqual(newDimension, m_yDimensionWidget->getDimension()))
  {
    if (pDimensionWidget != m_yDimensionWidget)
    {
      //Update the yDimensionWidget only.
      m_yDimensionWidget->populateWidget(oldDimensionIndex);
    }
  }

  if (areEqual(newDimension, m_zDimensionWidget->getDimension()))
  {
    if (pDimensionWidget != m_zDimensionWidget)
    {
      //Update the zDimensionWidget only.
      m_zDimensionWidget->populateWidget(oldDimensionIndex);
    }
  }

  //Update the zDimensionWidget only.
  if (areEqual(newDimension, m_tDimensionWidget->getDimension()))
  {
    if (pDimensionWidget != m_tDimensionWidget)
    {
      m_tDimensionWidget->populateWidget(oldDimensionIndex);
    }
  }

}
