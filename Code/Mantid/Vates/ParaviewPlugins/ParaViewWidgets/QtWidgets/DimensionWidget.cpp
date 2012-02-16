#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <qmessagebox.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include "DimensionWidget.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

using namespace Mantid::VATES;

DimensionWidget::DimensionWidget(bool readOnlyLimits)
{
  using namespace Mantid::Geometry;
  QGridLayout* m_layout = new QGridLayout();

  QLabel* integratedLabel = new QLabel("Int");
  integratedLabel->setToolTip("Collapse/Expand dimension");
  m_layout->addWidget(integratedLabel, 0, 0, Qt::AlignLeft);
  m_layout->setColumnMinimumWidth(0, 30); //Set column to fixed minimum width!

  m_ckIntegrated = new QCheckBox();
  m_ckIntegrated->setToolTip("Collapse/Expand dimension");
  connect(m_ckIntegrated, SIGNAL(clicked(bool)), this, SLOT(integratedChanged(bool)));
  m_layout->addWidget(m_ckIntegrated, 0, 1, Qt::AlignLeft);
  
  m_nBinsLabel = new QLabel("Bins");
  m_layout->addWidget(m_nBinsLabel, 0, 2, Qt::AlignLeft);
  m_nBinsBox = new QLineEdit();
  connect(m_nBinsBox, SIGNAL(editingFinished()), this, SLOT(nBinsListener()));
  m_layout->addWidget(m_nBinsBox, 0, 3, Qt::AlignLeft);

  m_dimensionLabel = new QLabel();
  m_dimensionLabel->setFixedWidth(100);
  m_layout->addWidget(m_dimensionLabel, 1, 0, Qt::AlignLeft);
  m_dimensionCombo = new QComboBox();

  connect(m_dimensionCombo,SIGNAL(activated(int)),this ,SLOT(dimensionSelectedListener()));
  m_layout->addWidget(m_dimensionCombo, 1, 1, Qt::AlignLeft);

  m_layout->addWidget(new QLabel("Min"), 1, 2, Qt::AlignLeft);
  m_minBox = new QLineEdit();

  connect(m_minBox, SIGNAL(editingFinished()), this, SLOT(minBoxListener()));
  m_layout->addWidget(m_minBox, 1, 3, Qt::AlignLeft);
  
  m_layout->addWidget(new QLabel("Max"), 1, 4, Qt::AlignLeft);
  m_maxBox = new QLineEdit();
  
  connect(m_maxBox, SIGNAL(editingFinished()), this, SLOT(maxBoxListener()));
  m_layout->addWidget(m_maxBox, 1, 5, Qt::AlignLeft);
  
  m_maxBox->setEnabled(!readOnlyLimits);
  m_minBox->setEnabled(!readOnlyLimits);

  this->setLayout(m_layout);
}


double DimensionWidget::getMinimum() const
{
  return atof(m_minBox->text().toStdString().c_str());
}

double DimensionWidget::getMaximum() const
{
  return atof(m_maxBox->text().toStdString().c_str());
}

unsigned int DimensionWidget::getNBins() const
{
  int nbins = static_cast<int>(m_pDimensionPresenter->getModel()->getNBins());
  int entry = atoi(m_nBinsBox->text());
  if(entry == nbins || entry <= 1)
  {
    std::string nBinsString = boost::str(boost::format("%i") % nbins);
    m_nBinsBox->setText(nBinsString.c_str());
  }
  return atoi(m_nBinsBox->text().toStdString().c_str());
}

void DimensionWidget::displayError(std::string message) const
{
    QMessageBox msgBox;
    msgBox.setText(message.c_str());
    msgBox.exec();
}

unsigned int DimensionWidget::getSelectedIndex() const
{
  return m_dimensionCombo->currentIndex();
}


void DimensionWidget::showAsNotIntegrated(Mantid::Geometry::VecIMDDimension_sptr)
{
  setDimensionName(m_pDimensionPresenter->getLabel());

  m_nBinsBox->setHidden(false);
  m_nBinsLabel->setHidden(false);
  m_ckIntegrated->setChecked(false);
  if(atoi(m_nBinsBox->text()) <= 1)
  {
    size_t modelBins = m_pDimensionPresenter->getModel()->getNBins();
    if( modelBins > 1)
    {
      m_nBinsBox->setText(boost::str(boost::format("%i") % modelBins).c_str());
    }
    else
    {
      m_nBinsBox->setText(boost::str(boost::format("%i") % 10).c_str());
    }

  }
}

/*
Helper method to set dimension names whereever required.
@param name : name of the dimension to display
*/
void DimensionWidget::setDimensionName(const std::string& name)
{
  m_dimensionLabel->setText(name.c_str());
  this->setToolTip(name.c_str());
}

  
void DimensionWidget::showAsIntegrated()
{
  setDimensionName(m_pDimensionPresenter->getModel()->getDimensionId());
  m_nBinsBox->setHidden(true);
  m_nBinsLabel->setHidden(true);
  m_ckIntegrated->setChecked(true);
}

/** Configure the DimensionView to override only selection choice controls. Otherwise leave nbins, max, min in their current state.
*/
void DimensionWidget::configureWeakly()
{
  using Mantid::Geometry::VecIMDDimension_sptr;
  m_dimensionCombo->clear();

  GeometryPresenter::MappingType mappings = m_pDimensionPresenter->getMappings(); //Should be sv collection?
  GeometryPresenter::MappingType::iterator it = mappings.begin();
  unsigned int count = 0;
  for(; it != mappings.end(); ++it)
  {
    m_dimensionCombo->addItem(it->first.c_str());
    if(it->first == m_pDimensionPresenter->getMapping())
    {
      m_dimensionCombo->setCurrentItem(count);
    }
    count++;

  }
}

/** Configure the DimensionView to override any controls with the values obtained from the model.
*/
void DimensionWidget::configureStrongly()
{
  configureWeakly();

  std::string nBinsString = boost::str(boost::format("%i") % m_pDimensionPresenter->getModel()->getNBins());
  m_nBinsBox->setText(nBinsString.c_str());

  std::string maxValueString = boost::str(boost::format("%i") % m_pDimensionPresenter->getModel()->getMaximum());
  m_maxBox->setText(maxValueString.c_str());

  std::string minValueString = boost::str(boost::format("%i") % m_pDimensionPresenter->getModel()->getMinimum());
  m_minBox->setText(minValueString.c_str());
 
}
      
void DimensionWidget::accept(Mantid::VATES::DimensionPresenter* pDimensionPresenter)
{
  m_pDimensionPresenter = pDimensionPresenter;
}

bool DimensionWidget::getIsIntegrated() const
{
  return m_ckIntegrated->isChecked();
}

void DimensionWidget::dimensionSelectedListener()
{
  m_pDimensionPresenter->updateModel();
}


void DimensionWidget::nBinsListener()
{
  m_pDimensionPresenter->updateModel();
}

void DimensionWidget::minBoxListener()
{
  m_pDimensionPresenter->updateModel();
}

void DimensionWidget::maxBoxListener()
{
  m_pDimensionPresenter->updateModel();
}

void DimensionWidget::integratedChanged(bool)
{
  try
  {
    m_pDimensionPresenter->updateModel();
  }
  catch(std::invalid_argument& ex)
  {
    m_ckIntegrated->setChecked(false);
    QMessageBox msgBox;
    msgBox.setText(ex.what());
    msgBox.exec();
  }
}

DimensionWidget::~DimensionWidget()
{

}

std::string DimensionWidget::getVisDimensionName() const
{
  if(m_dimensionCombo->isHidden())
  {
    return m_pDimensionPresenter->getMapping();
  }
  else
  {
    return m_dimensionCombo->currentText().toStdString(); 
  }
}

