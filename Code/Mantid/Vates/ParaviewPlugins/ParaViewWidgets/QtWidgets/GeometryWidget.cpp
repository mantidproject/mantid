#include "GeometryWidget.h"
#include "DimensionWidget.h"
#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MantidGeometry/MDGeometry/MDGeometry.h"
#include <boost/shared_ptr.hpp>
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

GeometryWidget::GeometryWidget() : m_isConstructed(false), m_xDimensionWidget(NULL), m_yDimensionWidget(NULL), m_zDimensionWidget(NULL), m_tDimensionWidget(NULL)
{
}

void GeometryWidget::constructWidget(std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> > nonIntegratedVector)
{

  m_nonIntegratedVector = nonIntegratedVector;
  delete layout();

  QGridLayout* layout = new QGridLayout;
  if(true == m_isConstructed) //Essentially assignment operation.
  {
    delete m_xDimensionWidget;
    delete m_yDimensionWidget;
    delete m_zDimensionWidget;
    delete m_tDimensionWidget;
  }
  unsigned int size = nonIntegratedVector.size();
  //Create widget to display/control the aligned x-dimension
  if(size > 0)
  {
    m_xDimensionWidget = new DimensionWidget(this, "x Dimension", 0, nonIntegratedVector);
    layout->addWidget(m_xDimensionWidget, 0, 0);
  }

  //Create widget to display/control the aligned y-dimension
  if(size > 1)
  {
    m_yDimensionWidget = new DimensionWidget(this, "y Dimension", 1, nonIntegratedVector);
    layout->addWidget(m_yDimensionWidget, 1, 0);
  }

  //Create widget to display/control the aligned z-dimension
  if(size > 2)
  {
    m_zDimensionWidget = new DimensionWidget(this, "z Dimension", 2, nonIntegratedVector);
    layout->addWidget(m_zDimensionWidget, 2, 0);
  }

  //Create widget to display/control the aligned t-dimension
  if(size > 3)
  {
    m_tDimensionWidget = new DimensionWidget(this, "t Dimension", 3, nonIntegratedVector);
    layout->addWidget(m_tDimensionWidget, 3, 0);
  }

  this->setLayout(layout);
  m_isConstructed = true;
}


QString GeometryWidget::getXDimensionXML() const
{
  validateSetup();
  //Get the selected alignment for the xdimension.
  if(hasXDimension())
  {
    return m_xDimensionWidget->getDimension()->toXMLString().c_str();
  }
  else
  {
    return "";
  }
}

QString GeometryWidget::getYDimensionXML() const
{
  validateSetup();
  if(hasYDimension())
  {
    return m_yDimensionWidget->getDimension()->toXMLString().c_str();
  }
  else
  {
    return "";
  }
}

QString GeometryWidget::getZDimensionXML() const
{
  validateSetup();
  if(hasZDimension())
  {
    return m_zDimensionWidget->getDimension()->toXMLString().c_str();
  }
  else
  {
    return "";
  }
}

QString GeometryWidget::gettDimensionXML() const
{
  validateSetup();
  if(hasTDimension())
  {
    return m_tDimensionWidget->getDimension()->toXMLString().c_str();
  }
  else
  {
    return "";
  }
}


GeometryWidget::~GeometryWidget()
{
}

void GeometryWidget::dimensionWidgetChanged(BinChangeStatus status)
{
  validateSetup();
  emit valueChanged();
  if(IgnoreBinChanges == status)
  {
    emit ignoreBinChanges();
  }
}

void GeometryWidget::applyBinsFromDimensions()
{
  if(hasXDimension())
  {
    m_xDimensionWidget->resetBins();
  }
  if(hasYDimension())
  {
   m_yDimensionWidget->resetBins();
  }
  if(hasZDimension())
  {
   m_zDimensionWidget->resetBins();
  }
  if(hasTDimension())
  {
   m_tDimensionWidget->resetBins();
  }
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

  if (hasXDimension() && isEqualToChangedDimension(m_xDimensionWidget->getDimension()))
  {
    if (pDimensionWidget != m_xDimensionWidget) //prevent self assigment.
    {
      //Update the xDimensionWidget only.
      m_xDimensionWidget->populateWidget(oldDimensionIndex);
    }
  }

  if (hasYDimension() && isEqualToChangedDimension(m_yDimensionWidget->getDimension()))
  {
    if (pDimensionWidget != m_yDimensionWidget) //prevent self assigment.
    {
      //Update the yDimensionWidget only.
      m_yDimensionWidget->populateWidget(oldDimensionIndex);
    }
  }

  if (hasZDimension() && isEqualToChangedDimension(m_zDimensionWidget->getDimension()))
  {
    if (pDimensionWidget != m_zDimensionWidget) //prevent self assigment.
    {
      //Update the zDimensionWidget only.
      m_zDimensionWidget->populateWidget(oldDimensionIndex);
    }
  }

  if (hasTDimension() && isEqualToChangedDimension(m_tDimensionWidget->getDimension()))
  {
    if (pDimensionWidget != m_tDimensionWidget) //prevent self assigment.
    {
      //Update the zDimensionWidget only.
      m_tDimensionWidget->populateWidget(oldDimensionIndex);
    }
  }
  applyBinsFromDimensions();
  //Raise event.
  dimensionWidgetChanged(IgnoreBinChanges);
}
