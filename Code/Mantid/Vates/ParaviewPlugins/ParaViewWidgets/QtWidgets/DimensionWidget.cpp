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
#include "DimensionWidget.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

DimensionWidget::DimensionWidget(bool readOnlyLimits)
{
  using namespace Mantid::Geometry;
  QGridLayout* m_layout = new QGridLayout();

  QLabel* integratedLabel = new QLabel("Integrated");
  m_layout->addWidget(integratedLabel, 0, 0, Qt::AlignLeft);

  m_ckIntegrated = new QCheckBox();
  connect(m_ckIntegrated, SIGNAL(clicked(bool)), this, SLOT(integratedChanged(bool)));
  m_layout->addWidget(m_ckIntegrated, 0, 1, Qt::AlignLeft);

  m_dimensionLabel = new QLabel();
  m_layout->addWidget(m_dimensionLabel, 1, 0, Qt::AlignLeft);
  m_dimensionCombo = new QComboBox();

  connect(m_dimensionCombo,SIGNAL(activated(int)),this ,SLOT(dimensionSelectedListener()));
  m_layout->addWidget(m_dimensionCombo, 1, 1, Qt::AlignLeft);

  m_nBinsLabel = new QLabel("Number of Bins");
  m_layout->addWidget(m_nBinsLabel, 2, 0, Qt::AlignLeft);
  m_nBinsBox = new QLineEdit();
  connect(m_nBinsBox, SIGNAL(editingFinished()), this, SLOT(nBinsListener()));
  m_layout->addWidget(m_nBinsBox, 2, 1, Qt::AlignLeft);
  
  QLabel* maxLabel = new QLabel("Maximum");
  maxLabel->setText("Maximum");
  m_layout->addWidget(maxLabel, 3, 0, Qt::AlignLeft);
  m_maxBox = new QLineEdit();

  connect(m_maxBox, SIGNAL(editingFinished()), this, SLOT(maxBoxListener()));
  m_layout->addWidget(m_maxBox, 3, 1, Qt::AlignLeft);

  QLabel* minLabel = new QLabel();
  minLabel->setText("Minimum");
  m_layout->addWidget(minLabel, 4, 0, Qt::AlignLeft);
  m_minBox = new QLineEdit();
  
  connect(m_minBox, SIGNAL(editingFinished()), this, SLOT(minBoxListener()));
  m_layout->addWidget(m_minBox, 4, 1, Qt::AlignLeft);

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

unsigned int DimensionWidget::getSelectedIndex() const
{
  return m_dimensionCombo->currentIndex();
}


void DimensionWidget::showAsNotIntegrated(Mantid::Geometry::VecIMDDimension_sptr nonIntegratedDims)
{
  m_dimensionLabel->setText(m_pDimensionPresenter->getLabel().c_str());
  m_nBinsBox->setHidden(false);
  m_dimensionCombo->setHidden(false);
  m_nBinsLabel->setHidden(false);
  m_ckIntegrated->setChecked(false);
  if(atoi(m_nBinsBox->text()) <= 1)
  {
    int modelBins = m_pDimensionPresenter->getModel()->getNBins();
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
  
void DimensionWidget::showAsIntegrated()
{
  m_dimensionLabel->setText(m_pDimensionPresenter->getModel()->getDimensionId().c_str());
  m_nBinsBox->setHidden(true);
  m_dimensionCombo->setHidden(true);
  m_nBinsLabel->setHidden(true);
  m_ckIntegrated->setChecked(true);
}

void DimensionWidget::configure()
{
  m_dimensionCombo->clear();
  Mantid::Geometry::VecIMDDimension_sptr vecNonIntegrated = m_pDimensionPresenter->getNonIntegratedDimensions();
  unsigned int vecSize = vecNonIntegrated.size();
  for(unsigned int i = 0; i < vecSize; i++)
  {
    boost::shared_ptr<Mantid::Geometry::IMDDimension> currentDimension = vecNonIntegrated[i];
    m_dimensionCombo->addItem( currentDimension->getDimensionId().c_str());
    if(currentDimension->getDimensionId() == m_pDimensionPresenter->getModel()->getDimensionId())
    {
      m_dimensionCombo->setCurrentItem(i);
    }
  }

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

void DimensionWidget::integratedChanged(bool checkedState)
{
  m_pDimensionPresenter->updateModel();
}

DimensionWidget::~DimensionWidget()
{

}

std::string DimensionWidget::getDimensionId() const
{
  if(m_dimensionCombo->isHidden())
  {
    return m_pDimensionPresenter->getModel()->getDimensionId();
  }
  else
  {
    return m_dimensionCombo->currentText().toStdString(); 
  }
}

