#include "GeometryWidget.h"
#include "DimensionWidget.h"
#include "MantidVatesAPI/GeometryPresenter.h"
#include <QLabel>
#include <QGridLayout>

/**
Constructor
@param pPresenter : pointer to MVP presenter
@param binDisplay : Enum describing how the bins should be displayed
*/
GeometryWidget::GeometryWidget(Mantid::VATES::GeometryPresenter* pPresenter, Mantid::VATES::BinDisplay binDisplay) : m_widgetFactory(binDisplay), m_pPresenter(pPresenter) 
{
  QGridLayout* headerLayout = new QGridLayout();
  QVBoxLayout* bodyLayout = new QVBoxLayout();
  
  headerLayout->addWidget(new QLabel("Geometry"), 0, 0, 1, 2, Qt::AlignCenter); 
  bodyLayout->addLayout(headerLayout);
  
  this->setLayout(bodyLayout);
  m_pPresenter->acceptView(this);
}

/// Destructor
GeometryWidget::~GeometryWidget()
{
  delete m_pPresenter;
}

/**
Add a new dimension view.
@param dimView : dimensionview (widget) to add to overall geometry widget. 
*/
void GeometryWidget::addDimensionView(Mantid::VATES::DimensionView* dimView)
{
  DimensionWidget* dimWidget = dynamic_cast<DimensionWidget*>(dimView); //TODO. design should not need capability queries!
  if(dimWidget != NULL)
  {
    QLayout* layout = this->layout();
    layout->addWidget(dimWidget);
  }
}

/**
Getter for the resultant/current xml string.
@return xml as a string.
*/
std::string GeometryWidget::getGeometryXMLString() const
{
  return m_pPresenter->getGeometryXML();
}

/*
Gets a ref to the dimension view factory
Allows new dimensions of a type compatible with this GeometryWidget to be fabricated.
*/
const Mantid::VATES::DimensionViewFactory& GeometryWidget::getDimensionViewFactory()
{
  return m_widgetFactory;
}

/*
Indicate that the geometry widget has been modified by emitting an event.
*/
void GeometryWidget::raiseModified()
{
  emit valueChanged();
}
