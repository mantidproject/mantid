#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <qmessagebox.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include "GeometryWidget.h"
#include "DimensionWidget.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include "MantidVatesAPI/RebinningCutterPresenter.h"
#include "MantidMDAlgorithms/DimensionFactory.h"

DimensionWidget::DimensionWidget(
    GeometryWidget* geometryWidget,
    const std::string& name,
    const int dimensionIndex,
    std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> > vecNonIntegratedDimensions )
:
    m_layout(NULL),
    m_nBinsBox(NULL),
    m_minBox(NULL),
    m_maxBox(NULL),
    m_dimensionCombo(NULL),
    m_currentDimensionIndex(0),
    m_name(name),
    m_geometryWidget(geometryWidget),
    m_vecNonIntegratedDimensions(vecNonIntegratedDimensions)
{
  constructWidget(dimensionIndex);
  populateWidget(dimensionIndex);
}

void DimensionWidget::constructWidget(const int dimensionIndex)
{
  using namespace Mantid::Geometry;

  boost::shared_ptr<IMDDimension> spDimensionToRender = m_vecNonIntegratedDimensions[dimensionIndex];
  QGridLayout* m_layout = new QGridLayout();

  QLabel* dimensionLabel = new QLabel();
  dimensionLabel->setText(m_name.c_str());
  m_layout->addWidget(dimensionLabel, 0, 0, Qt::AlignLeft);
  m_dimensionCombo = new QComboBox();

  //Loop through non-integrated dimensions and add them to the combobox.
  for(unsigned int i = 0; i < m_vecNonIntegratedDimensions.size(); i++)
  {
    boost::shared_ptr<Mantid::Geometry::IMDDimension> currentDimension = m_vecNonIntegratedDimensions[i];
    m_dimensionCombo->addItem( currentDimension->getName().c_str());
    if(currentDimension->getDimensionId() == spDimensionToRender->getDimensionId())
    {
      m_dimensionCombo->setCurrentItem(i);
    }
  }

  connect(m_dimensionCombo,SIGNAL(currentIndexChanged(int)),this ,SLOT(dimensionSelectedListener()));
  m_layout->addWidget(m_dimensionCombo, 0, 1, Qt::AlignLeft);

  QLabel* nBinsLabel = new QLabel();
  nBinsLabel->setText("Number of Bins");
  m_layout->addWidget(nBinsLabel, 1, 0, Qt::AlignLeft);
  m_nBinsBox = new QLineEdit();
  connect(m_nBinsBox, SIGNAL(editingFinished()), this, SLOT(nBinsListener()));
  m_layout->addWidget(m_nBinsBox, 1, 1, Qt::AlignLeft);

  QLabel* maxLabel = new QLabel("Maximum");
  maxLabel->setText("Maximum");
  m_layout->addWidget(maxLabel, 2, 0, Qt::AlignLeft);
  m_maxBox = new QLineEdit();
  //m_maxBox->setEnabled(false);
  connect(m_maxBox, SIGNAL(editingFinished()), this, SLOT(maxBoxListener()));
  m_layout->addWidget(m_maxBox, 2, 1, Qt::AlignLeft);

  QLabel* minLabel = new QLabel();
  minLabel->setText("Minimum");
  m_layout->addWidget(minLabel, 3, 0, Qt::AlignLeft);
  m_minBox = new QLineEdit();
  //m_minBox->setEnabled(false);
  connect(m_minBox, SIGNAL(editingFinished()), this, SLOT(minBoxListener()));
  m_layout->addWidget(m_minBox, 3, 1, Qt::AlignLeft);

  this->setLayout(m_layout);
}

void DimensionWidget::populateWidget(const int dimensionIndex)
{
  //Essentially check that the construct method has actually been called first.
  if (m_dimensionCombo != NULL)
  {
    using namespace Mantid::Geometry;
    boost::shared_ptr<IMDDimension> spDimensionToRender = m_vecNonIntegratedDimensions[dimensionIndex];

    m_currentDimensionIndex = dimensionIndex;
    m_dimensionCombo->setCurrentIndex(dimensionIndex);

    if (m_nBinsBox->text().isEmpty())
    {
      std::string nBinsString = boost::str(boost::format("%i") % spDimensionToRender->getNBins());
      m_nBinsBox->setText(nBinsString.c_str());
    }

    if( m_maxBox->text().isEmpty())
    {
      std::string maxValueString = boost::str(boost::format("%i") % spDimensionToRender->getMaximum());
      m_maxBox->setText(maxValueString.c_str());
    }

    if( m_minBox->text().isEmpty())
    {
      std::string minValueString = boost::str(boost::format("%i") % spDimensionToRender->getMinimum());
      m_minBox->setText(minValueString.c_str());
    }
  }
}

double DimensionWidget::getMinimum() const
{
  return atof(m_minBox->text().toStdString().c_str());
}

double DimensionWidget::getMaximum() const
{
  return atof(m_maxBox->text().toStdString().c_str());
}

int DimensionWidget::getNBins()
{
  int nbins = static_cast<int>( m_vecNonIntegratedDimensions[m_currentDimensionIndex]->getNBins() );
  int entry = atoi(m_nBinsBox->text());
  if(entry == nbins || entry <= 1)
  {
    resetBins();
  }
  return atoi(m_nBinsBox->text().toStdString().c_str());
}

int DimensionWidget::getSelectedIndex() const
{
  return m_dimensionCombo->currentIndex();
}

void DimensionWidget::setMinimum(double minimum)
{
  std::string minValueString = boost::str(boost::format("%i") % minimum);
  m_minBox->setText(minValueString.c_str());
}

void DimensionWidget::setMaximum(double maximum)
{
  std::string maxValueString = boost::str(boost::format("%i") % maximum);
  m_maxBox->setText(maxValueString.c_str());
}

boost::shared_ptr<Mantid::Geometry::IMDDimension>  DimensionWidget::getDimension()
{
  boost::shared_ptr<Mantid::Geometry::IMDDimension> originalDimension = m_vecNonIntegratedDimensions[m_currentDimensionIndex];
  //Remake the dimension with a new number of bins. Note: Would be much better to have the clone facility.

  return Mantid::MDAlgorithms::createDimension(originalDimension->toXMLString(), this->getNBins(), this->getMinimum(), this->getMaximum());
}

void DimensionWidget::dimensionSelectedListener()
{
  //Providing that the current item has not been reselected, populate/repopulate the widget with
  //Dimension information from the newly selected dimension.
  if (m_dimensionCombo->currentIndex() != m_currentDimensionIndex)
  {
    using namespace Mantid::Geometry;

    const int oldDimensionIndex = m_currentDimensionIndex;

    boost::shared_ptr<IMDDimension> spNewDimension =  m_vecNonIntegratedDimensions[m_dimensionCombo->currentIndex()];

    int dimensionIndex = m_dimensionCombo->currentIndex();
    populateWidget(dimensionIndex);
    m_geometryWidget->childAppliedNewDimensionSelection(oldDimensionIndex, spNewDimension, this);
  }
}

void DimensionWidget::resetBins()
{
  int nbins = static_cast<int>( m_vecNonIntegratedDimensions[m_currentDimensionIndex]->getNBins() );
  std::string nBinsString = boost::str(boost::format("%i") % nbins);
  m_nBinsBox->setText(nBinsString.c_str());
}

void DimensionWidget::nBinsListener()
{
  int nbins = static_cast<int>( m_vecNonIntegratedDimensions[m_currentDimensionIndex]->getNBins() );
  int entry = atoi(m_nBinsBox->text());
  if(entry == nbins || entry <= 1)
  {
    resetBins();
  }
  {
    m_geometryWidget->dimensionWidgetChanged(ApplyBinChanges);
  }
}

void DimensionWidget::minBoxListener()
{
  m_geometryWidget->dimensionWidgetChanged(ApplyBinChanges);
}

void DimensionWidget::maxBoxListener()
{
  m_geometryWidget->dimensionWidgetChanged(ApplyBinChanges);
}

DimensionWidget::~DimensionWidget()
{

}
