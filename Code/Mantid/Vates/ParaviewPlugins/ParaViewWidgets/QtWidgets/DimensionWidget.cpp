#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QSpacerItem>
#include <QStackedWidget>
#include <qmessagebox.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include "DimensionWidget.h"
#include "LowHighStepInputWidget.h"
#include "SimpleBinInputWidget.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/scoped_ptr.hpp>

using namespace Mantid::VATES;

/**
Constructor
*/
DimensionWidget::DimensionWidget() :
  m_layout(NULL), m_binLayout(NULL), m_axisLayout(NULL), m_currentDimensionIndex(0),
  m_currentBinWidgetIndex(0), m_pDimensionPresenter(NULL), m_initialBinDisplay(BinDisplay::Simple)
{
  m_binStackedWidget = new QStackedWidget;
  BinInputWidget* simple = new SimpleBinInputWidget;
  BinInputWidget* lowstephigh = new LowHighStepInputWidget;
  m_binStackedWidget->addWidget(simple);
  m_binStackedWidget->addWidget(lowstephigh);
  m_binStackedWidget->addWidget(new QLabel(""));
  m_binStackedWidget->setCurrentIndex(m_currentBinWidgetIndex);

  using namespace Mantid::Geometry;
  QVBoxLayout* m_layout = new QVBoxLayout();
  m_layout->setSpacing(2);

  m_dimensionLabel = new QLabel();
  m_layout->addWidget(m_dimensionLabel, Qt::AlignLeft);

  QHBoxLayout* m_binLayout = new QHBoxLayout();

  m_ckIntegrated = new QCheckBox();
  m_ckIntegrated->setText("Integrate");
  m_ckIntegrated->setToolTip("Collapse/Expand dimension");
  connect(m_ckIntegrated, SIGNAL(clicked(bool)), this, SLOT(integratedChanged(bool)));
  m_binLayout->addWidget(m_ckIntegrated);

  QSpacerItem* spacer = new QSpacerItem(40, 20,
                                        QSizePolicy::Maximum,
                                        QSizePolicy::Minimum);
  m_binLayout->addSpacerItem(spacer);

  m_binLayout->addWidget(m_binStackedWidget, Qt::AlignLeft);
  connect(simple, SIGNAL(valueChanged()), this, SLOT(nBinsListener()));
  connect(lowstephigh, SIGNAL(valueChanged()), this, SLOT(nBinsListener()));

  m_layout->addLayout(m_binLayout);

  QHBoxLayout* m_axisLayout = new QHBoxLayout();

  m_dimensionCombo = new QComboBox();
  QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
  m_dimensionCombo->setSizePolicy(sizePolicy);
  m_dimensionCombo->setMinimumSize(QSize(80, 0));
  connect(m_dimensionCombo,SIGNAL(activated(int)),this ,SLOT(dimensionSelectedListener()));
  m_axisLayout->addWidget(m_dimensionCombo, Qt::AlignLeft);

  m_axisLayout->addWidget(new QLabel("Min"));

  m_minBox = new QLineEdit();
  m_minBox->setValidator(new QDoubleValidator(this));
  QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Minimum);
  m_minBox->setSizePolicy(sizePolicy1);
  m_minBox->setMinimumSize(QSize(50, 0));
  connect(m_minBox, SIGNAL(editingFinished()), this, SLOT(minBoxListener()));
  m_axisLayout->addWidget(m_minBox, Qt::AlignLeft);

  m_axisLayout->addWidget(new QLabel("Max"));

  m_maxBox = new QLineEdit();
  m_maxBox->setValidator(new QDoubleValidator(this));
  QSizePolicy sizePolicy2(QSizePolicy::Minimum, QSizePolicy::Minimum);
  m_maxBox->setSizePolicy(sizePolicy2);
  m_maxBox->setMinimumSize(QSize(50, 0));
  connect(m_maxBox, SIGNAL(editingFinished()), this, SLOT(maxBoxListener()));
  m_axisLayout->addWidget(m_maxBox, Qt::AlignLeft);

  m_layout->addLayout(m_axisLayout);

  this->setLayout(m_layout);
}

void DimensionWidget::initalizeViewMode(BinDisplay binDisplay)
{
  m_initialBinDisplay = binDisplay;
}


BinInputWidget* DimensionWidget::getCurrentBinInputWidget() const
{
  QWidget *w;
  if (m_binStackedWidget->currentIndex() > 1)
  {
    w = m_binStackedWidget->widget(m_currentBinWidgetIndex);
  }
  else
  {
    w = m_binStackedWidget->currentWidget();
  }
  return dynamic_cast<BinInputWidget*>(w);
}

Mantid::coord_t DimensionWidget::getMinimum() const
{
  return m_minBox->text().toFloat();
}

Mantid::coord_t DimensionWidget::getMaximum() const
{
  return m_maxBox->text().toFloat();
}

unsigned int DimensionWidget::getNBins() const
{
  int nbins = static_cast<int>(m_pDimensionPresenter->getModel()->getNBins());
  double max = m_pDimensionPresenter->getModel()->getMaximum();
  double min = m_pDimensionPresenter->getModel()->getMinimum();
  BinInputWidget* binInputWidget = getCurrentBinInputWidget();
  int entry = binInputWidget->getEntry(min, max);
  if(entry == nbins || entry <= 1)
  {
    binInputWidget->setEntry(nbins, min, max);
  }
  return binInputWidget->getEntry(min, max);
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
  double max = m_pDimensionPresenter->getModel()->getMaximum();
  double min = m_pDimensionPresenter->getModel()->getMinimum();
  m_binStackedWidget->setCurrentIndex(m_currentBinWidgetIndex);
  m_ckIntegrated->setChecked(false);
  BinInputWidget* binInputWidget = getCurrentBinInputWidget();
  if(binInputWidget->getEntry(min, max) <= 1)
  {
    size_t modelBins = m_pDimensionPresenter->getModel()->getNBins();
    if( modelBins > 1)
    {
      binInputWidget->setEntry(int(modelBins), min, max);
    }
    else
    {
      binInputWidget->setEntry(10, min, max);
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
  m_binStackedWidget->setCurrentIndex(2);
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
  double max = m_pDimensionPresenter->getModel()->getMaximum();
  double min = m_pDimensionPresenter->getModel()->getMinimum();
  BinInputWidget* binInputWidget = getCurrentBinInputWidget();
  binInputWidget->setEntry(int(m_pDimensionPresenter->getModel()->getNBins()),min,max);

  std::string maxValueString = boost::str(boost::format("%i") % m_pDimensionPresenter->getModel()->getMaximum());
  m_maxBox->setText(maxValueString.c_str());

  std::string minValueString = boost::str(boost::format("%i") % m_pDimensionPresenter->getModel()->getMinimum());
  m_minBox->setText(minValueString.c_str());
  setViewMode(m_initialBinDisplay);
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

void DimensionWidget::setViewMode(Mantid::VATES::BinDisplay mode)
{
  double max = m_pDimensionPresenter->getModel()->getMaximum();
  double min = m_pDimensionPresenter->getModel()->getMinimum();
  BinInputWidget* binInputWidget = getCurrentBinInputWidget();
  int nBins = binInputWidget->getEntry(min, max);

  if(mode == Simple)
  {
    m_currentBinWidgetIndex = 0;
    if (!m_ckIntegrated->isChecked())
    {
      m_binStackedWidget->setCurrentIndex(m_currentBinWidgetIndex);
    }
  }
  else if(mode == LowHighStep)
  {
    m_currentBinWidgetIndex = 1;
    if (!m_ckIntegrated->isChecked())
    {
      m_binStackedWidget->setCurrentIndex(m_currentBinWidgetIndex);
    }
  }
  else
  {
    throw std::invalid_argument("Unknown bin display mode.");
  }
  BinInputWidget* binWidget = getCurrentBinInputWidget();
  binWidget->setEntry(nBins, min, max);
}

