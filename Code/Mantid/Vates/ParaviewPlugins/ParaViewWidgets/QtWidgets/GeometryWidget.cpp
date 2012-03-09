#include "GeometryWidget.h"
#include "DimensionWidget.h"
#include "MantidVatesAPI/GeometryPresenter.h"
#include <QLabel>
#include <QGridLayout>
#include <QCheckBox>

using namespace Mantid::VATES;

/**
Constructor
@param pPresenter : pointer to MVP presenter
@param binDisplay : Enum describing how the bins should be displayed
*/
GeometryWidget::GeometryWidget(Mantid::VATES::GeometryPresenter* pPresenter, BinDisplay binDisplay) : m_widgetFactory(binDisplay), m_pPresenter(pPresenter), m_ckBinDisplay(new QCheckBox)
{
  m_ckBinDisplay->setText("By Number of Bins");
  m_ckBinDisplay->setToolTip("Specify the exact number of bins or a step in a low, high step schenario");
  m_ckBinDisplay->setChecked(true);
  connect(m_ckBinDisplay, SIGNAL(clicked(bool)), this, SLOT(binModeChanged(bool)));

  QGridLayout* headerLayout = new QGridLayout();
  QVBoxLayout* bodyLayout = new QVBoxLayout();
  
  headerLayout->addWidget(new QLabel("Geometry"), 0, 0, 1, 2, Qt::AlignCenter); 
  
  QCheckBox* box = new QCheckBox;
  bodyLayout->addWidget(m_ckBinDisplay);
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
void GeometryWidget::addDimensionView(DimensionView* dimView)
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

/**
Indicate that the geometry widget has been modified by emitting an event.
*/
void GeometryWidget::raiseModified()
{
  emit valueChanged();
}

/**
Handle changes in the binning mode.
*/
void GeometryWidget::binModeChanged(bool)
{
  this->m_pPresenter->setDimensionModeChanged();
}

/**
Getter to indicate whether the number of bins should be used
@return BinDisplayMode to use.
*/
BinDisplay GeometryWidget::getBinDisplayMode() const
{
  bool useNumberOfBins = this->m_ckBinDisplay->checkState();
  return useNumberOfBins ? Simple : LowHighStep;
}
