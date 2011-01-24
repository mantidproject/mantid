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
  bool operator()(boost::shared_ptr<IMDDimension> a, boost::shared_ptr<IMDDimension> b) const
  {
    return a->getDimensionId() == b->getDimensionId();
  }
};

void GeometryWidget::validateSetup() const
{
  if(!m_isConstructed)
  {
    throw new std::runtime_error("Must geometry widget before doing anything else.");
  }
}

GeometryWidget::GeometryWidget() : m_isConstructed(false)
{
}

void GeometryWidget::constructWidget(std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> > nonIntegratedVector)
{

  m_nonIntegratedVector = nonIntegratedVector;
  delete layout();

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
  m_isConstructed = true;


}


std::string GeometryWidget::getXDimensionXML() const
{
  validateSetup();
  //Get the selected alignment for the xdimension.
  return m_xDimensionWidget->getDimension()->toXMLString();
}

std::string GeometryWidget::getYDimensionXML() const
{
  validateSetup();
  return m_yDimensionWidget->getDimension()->toXMLString();
}

std::string GeometryWidget::getZDimensionXML() const
{
  validateSetup();
  return m_zDimensionWidget->getDimension()->toXMLString();
}

std::string GeometryWidget::gettDimensionXML() const
{
  validateSetup();
  return m_tDimensionWidget->getDimension()->toXMLString();
}


GeometryWidget::~GeometryWidget()
{
}

void GeometryWidget::dimensionWidgetChanged()
{
  validateSetup();
  emit valueChanged();
}

void GeometryWidget::childAppliedNewDimensionSelection(const unsigned int oldDimensionIndex,
    boost::shared_ptr<IMDDimension> newDimension, DimensionWidget* pDimensionWidget)
{
  validateSetup();
  //Updates all child guis with overwrite dimension.
  using namespace Mantid::Geometry;

  //Comparitor
  DimensionCompare areEqual;
  std::binder1st<DimensionCompare> isEqualToChangedDimension(areEqual, newDimension);


  //The new Dimension is overwriting the dimension on this widget.
  //Assign the old widget the old dimension from the calling widget.

  if (isEqualToChangedDimension(m_xDimensionWidget->getDimension()))
  {
    if (pDimensionWidget != m_xDimensionWidget)
    {
      //Update the xDimensionWidget only.
      m_xDimensionWidget->populateWidget(oldDimensionIndex);
    }
  }

  if (isEqualToChangedDimension(m_yDimensionWidget->getDimension()))
  {
    if (pDimensionWidget != m_yDimensionWidget)
    {
      //Update the yDimensionWidget only.
      m_yDimensionWidget->populateWidget(oldDimensionIndex);
    }
  }

  if (isEqualToChangedDimension(m_zDimensionWidget->getDimension()))
  {
    if (pDimensionWidget != m_zDimensionWidget)
    {
      //Update the zDimensionWidget only.
      m_zDimensionWidget->populateWidget(oldDimensionIndex);
    }
  }

  //Update the zDimensionWidget only.
  if (isEqualToChangedDimension(m_tDimensionWidget->getDimension()))
  {
    if (pDimensionWidget != m_tDimensionWidget)
    {
      m_tDimensionWidget->populateWidget(oldDimensionIndex);
    }
  }

  //Raise event.
  dimensionWidgetChanged();
}
