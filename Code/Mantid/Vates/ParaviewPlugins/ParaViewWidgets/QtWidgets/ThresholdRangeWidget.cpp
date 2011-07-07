#include "ThresholdRangeWidget.h"
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
//#include <QBoxLayout>
#include <QCheckBox>
#include <QPalette>
#include <QFont>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

ThresholdRangeWidget::ThresholdRangeWidget(double min, double max) 
{

  QGridLayout* layout = new QGridLayout();
  layout->addWidget(new QLabel("Threshold Ranges"), 0 , 0 , 1, 2, Qt::AlignCenter);
  QLabel* thresholdMethodLabel = new QLabel("User Defined");
  layout->addWidget(thresholdMethodLabel, 1, 0, Qt::AlignLeft);
  m_ckUserDefined = new QCheckBox();
  m_ckUserDefined->setChecked(false); //Automatic selection by default.
  connect(m_ckUserDefined, SIGNAL(clicked(bool)), this, SLOT(methodChanged(bool)));
  layout->addWidget(m_ckUserDefined, 1, 1, Qt::AlignLeft);

  m_minLabel = new QLabel("Min signal");
  m_minEditBox = new QLineEdit();
  std::string minValueString = boost::str(boost::format("%0.2f") % min);
  m_minEditBox->setText(minValueString.c_str());
  layout->addWidget(m_minLabel, 2, 0, Qt::AlignLeft);
  layout->addWidget(m_minEditBox, 2, 1, Qt::AlignLeft);
  m_minEditBox->setDisabled(true); //Disabled by default.
  connect(m_minEditBox, SIGNAL(textEdited(const QString &)), this, SLOT(minThresholdListener(const QString &)));

  m_maxLabel = new QLabel("Max signal");
  m_maxEditBox = new QLineEdit();
  std::string maxValueString = boost::str(boost::format("%0.2f") % max);
  m_maxEditBox->setText(maxValueString.c_str());
  m_maxEditBox->setDisabled(true); //Disabled by default
  layout->addWidget(m_maxLabel, 3, 0, Qt::AlignLeft);
  layout->addWidget(m_maxEditBox, 3, 1, Qt::AlignLeft);
  connect(m_maxEditBox, SIGNAL(textEdited(const QString &)), this, SLOT(maxThresholdListener(const QString &)));

  this->setLayout(layout);
}

void ThresholdRangeWidget::maxThresholdListener(const QString &)
{
  emit maxChanged();
}

void ThresholdRangeWidget::minThresholdListener(const QString &)
{
  emit minChanged();
}

void ThresholdRangeWidget::setMaximum(double value)
{
  std::string maxValueString = boost::str(boost::format("%0.2f") % value);
  m_maxEditBox->setText(maxValueString.c_str());
}

void ThresholdRangeWidget::setMinimum(double value)
{
  std::string minValueString = boost::str(boost::format("%0.2f") % value);
  m_minEditBox->setText(minValueString.c_str());
}

ThresholdRangeWidget::~ThresholdRangeWidget()
{
}

QString ThresholdRangeWidget::getMaxSignal() const
{
  return m_maxEditBox->text();
}

QString ThresholdRangeWidget::getMinSignal() const
{
  return m_minEditBox->text();
}

bool ThresholdRangeWidget::getUserDefinedRange() const
{
  return m_ckUserDefined->isChecked();
}

void ThresholdRangeWidget::methodChanged(bool)
{
  bool disableUserControls;
  if(m_ckUserDefined->isChecked())
  {
    disableUserControls = false;
  }
  else
  {
    disableUserControls = true;
  } 
  m_maxEditBox->setDisabled(disableUserControls);
  m_minEditBox->setDisabled(disableUserControls);
  emit userDefinedChanged(true);
}
