#include "GeometryWidget.h"
#include "DimensionWidget.h"
#include "MantidGeometry/MDGeometry/MDGeometryXMLBuilder.h"
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

GeometryWidget::GeometryWidget(DimensionLimitsOption limitsOption) : 
  m_xDimensionWidget(NULL), 
  m_yDimensionWidget(NULL), 
  m_zDimensionWidget(NULL), 
  m_tDimensionWidget(NULL), 
  m_isConstructed(false), 
  m_limitsOption(limitsOption)
{
}

void GeometryWidget::constructWidget(Mantid::VATES::GeometryXMLParser& source)
{
  source.execute();
  delete layout();

  QGridLayout* layout = new QGridLayout;
  if(true == m_isConstructed) //Essentially assignment operation.
  {
    delete m_xDimensionWidget;
    delete m_yDimensionWidget;
    delete m_zDimensionWidget;
    delete m_tDimensionWidget;
  }
  Mantid::Geometry::VecIMDDimension_sptr nonIntegratedVector = source.getNonIntegratedDimensions();

  //Create widget to display/control the aligned x-dimension
  if(source.hasXDimension())
  {
    m_xDimensionWidget = new DimensionWidget(this, "x Dimension", 0, nonIntegratedVector, m_limitsOption);
    layout->addWidget(m_xDimensionWidget, 0, 0);
  }

  //Create widget to display/control the aligned y-dimension
  if(source.hasYDimension())
  {
    m_yDimensionWidget = new DimensionWidget(this, "y Dimension", 1, nonIntegratedVector, m_limitsOption);
    layout->addWidget(m_yDimensionWidget, 1, 0);
  }

  //Create widget to display/control the aligned z-dimension
  if(source.hasZDimension())
  {
    m_zDimensionWidget = new DimensionWidget(this, "z Dimension", 2, nonIntegratedVector, m_limitsOption);
    layout->addWidget(m_zDimensionWidget, 2, 0);
  }

  //Create widget to display/control the aligned t-dimension
  if(source.hasTDimension())
  {
    m_tDimensionWidget = new DimensionWidget(this, "t Dimension", 3, nonIntegratedVector, m_limitsOption);
    layout->addWidget(m_tDimensionWidget, 3, 0);
  }

  this->setLayout(layout);
  m_isConstructed = true;
}


QString GeometryWidget::getGeometryXML() const
{
  validateSetup();
  //Get the selected alignment for the xdimension.
  using namespace Mantid::Geometry;
  MDGeometryBuilderXML<StrictDimensionPolicy> xmlBuilder;
  if(hasXDimension())
  {
    xmlBuilder.addXDimension(m_xDimensionWidget->getDimension());
  }
  if(hasYDimension())
  {
    xmlBuilder.addYDimension(m_yDimensionWidget->getDimension());
  }
  if(hasZDimension())
  {
    xmlBuilder.addZDimension(m_zDimensionWidget->getDimension());
  }
  if(hasTDimension())
  {
    xmlBuilder.addTDimension(m_tDimensionWidget->getDimension());
  }
  return xmlBuilder.create().c_str();
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
