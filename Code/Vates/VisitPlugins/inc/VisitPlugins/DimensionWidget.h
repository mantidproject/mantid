#ifndef DIMENSION_WIDGET_H
#define DIMENSION_WIDGET_H

#include <qgridlayout.h>
#include <qwidget.h>
#include <memory>

//Foward decs
class QLabel;
class QComboBox;
class IntegratedDimensionWidget;
class DimensionPickerWidget;

namespace Mantid
{
 namespace MDAlgorithms
 {
 class DimensionParameterSet;
 class DimensionParameter;
 class DimensionParameterIntegration;
 }
}


class DimensionWidget: public QWidget
{
Q_OBJECT
public:
  DimensionWidget(Mantid::MDAlgorithms::DimensionParameterSet* set); //TODO, widget should have ref to Dimensionset

  ~DimensionWidget()
  {
  }

private:

  void construct(std::auto_ptr<Mantid::MDAlgorithms::DimensionParameterSet> set);
  std::auto_ptr<Mantid::MDAlgorithms::DimensionParameterSet> m_set;
  QComboBox* m_DimensionPicker;
  std::vector<std::string> m_dimensions;
};

#endif
