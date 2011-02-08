#ifndef INTEGRATED_DIMENSION_WIDGET_H
#define INTEGRATED_DIMENSION_WIDGET_H

#include <qgridlayout.h>
#include <qwidget.h>

class RebinningCutterAttributes;
class QLabel;
class QLineEdit;

class IntegratedDimensionWidget: public QWidget
{
Q_OBJECT
public:
  IntegratedDimensionWidget(std::string dimensionName, double min, double max); //TODO, widget should have ref to Dimension

  ~IntegratedDimensionWidget()
  {
  }
  double getLowerLimit() const;
  double getUpperLimit() const;
private:
  QLineEdit* m_LowerLimitInput;
  QLineEdit* m_UpperLimitInput;
};

#endif
