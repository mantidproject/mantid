#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <stdio.h>
#include "IntegratedDimensionWidget.h"
//Custom warning
#warning "Custom Warning. Incomplete implementation. Does not use Mantid Dimension Domain types for construction!"

IntegratedDimensionWidget::IntegratedDimensionWidget(std::string dimensionName, double min, double max) :
  QWidget() //TODO Const ptr to dimension should be passed in instead.
{
  QGridLayout* layout = new QGridLayout();

  QLabel* titleLabel = new QLabel(dimensionName.c_str());
  layout->addWidget(titleLabel, 0, 0, 1, 2, Qt::AlignLeft);

  char buffer[255];
  sprintf(buffer, "Min = %.2f, Max = %.2f", min, max);
  QLabel* minMaxLabel = new QLabel(buffer);
  layout->addWidget(minMaxLabel, 1, 0, 1, 2, Qt::AlignLeft);

  QLabel* lowerLimitLabel = new QLabel("Lower Limit");
  m_LowerLimitInput = new QLineEdit;
  sprintf(buffer, "%.2f", min);
  m_LowerLimitInput->setText(buffer);
  layout->addWidget(lowerLimitLabel, 2, 0);
  layout->addWidget(m_LowerLimitInput, 2, 1);

  QLabel* upperLimitLabel = new QLabel("Upper Limit");
  m_UpperLimitInput = new QLineEdit;
  sprintf(buffer, "%.2f", max);
  m_UpperLimitInput->setText(buffer);
  layout->addWidget(upperLimitLabel, 3, 0);
  layout->addWidget(m_UpperLimitInput, 3, 1);

  setLayout(layout);
}

double IntegratedDimensionWidget::getUpperLimit() const
{
  return atof(m_UpperLimitInput->text().toStdString().c_str());
}

double IntegratedDimensionWidget::getLowerLimit() const
{
  return atof(m_LowerLimitInput->text().toStdString().c_str());
}

