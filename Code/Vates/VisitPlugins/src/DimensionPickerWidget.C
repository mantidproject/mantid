#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QComboBox>
#include <stdio.h>
#include <string>
#include "DimensionPickerWidget.h"

//Custom warning
#warning "Custom Warning. Incomplete implementation. Does not use Mantid Dimension Domain types for construction!"

DimensionPickerWidget::DimensionPickerWidget(std::string dimensionName,
    std::vector<std::string> dimensions)
{
  QGridLayout* layout = new QGridLayout();

  QLabel* titleLabel = new QLabel(dimensionName.c_str());
  layout->addWidget(titleLabel, 0, 0);

  m_DimensionPicker = new QComboBox;
  std::vector<std::string>::iterator it = dimensions.begin();
  for(it; it != dimensions.end(); ++it)
  {
    m_DimensionPicker->addItem((*it).c_str());
  }

  layout->addWidget(m_DimensionPicker, 0, 1);
  setLayout(layout);
}

int DimensionPickerWidget::getSelectedDimensionId() const
{
  m_DimensionPicker->currentIndex(); //TODO should get and return the selected dimension.
}

