#ifndef DIMENSION_PICKER_WIDGET_H
#define DIMENSION_PICKER_WIDGET_H

#include <qgridlayout.h>
#include <qwidget.h>

class QLabel;
class QComboBox;

class DimensionPickerWidget: public QWidget
{
Q_OBJECT
public:
  DimensionPickerWidget(std::string dimensionName, std::vector<std::string> dimensions); //TODO, widget should have ref to Dimensionset

  ~DimensionPickerWidget()
  {
  }
  int getSelectedDimensionId() const;
private:
  QComboBox* m_DimensionPicker;
  std::vector<std::string> m_dimensions;
};

#endif
