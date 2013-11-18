#ifndef _DIMENSION_WIDGET_FACTORY_H
#define _DIMENSION_WIDGET_FACTORY_H

#include "WidgetDllOption.h"
#include "MantidVatesAPI/DimensionViewFactory.h"

/**
class DimensionWidgetFactory
concrete DimensionViewFactory. Creational type, fabricating dimension widgets on request.
*/
// cppcheck-suppress class_X_Y
class EXPORT_OPT_MANTIDPARVIEW DimensionWidgetFactory  : public Mantid::VATES::DimensionViewFactory
{
public:
  /// Constructor
  DimensionWidgetFactory(Mantid::VATES::BinDisplay binDisplay);
  /// Construction method.
  Mantid::VATES::DimensionView* create() const;
private:
  /// Bin display configuration.
  const Mantid::VATES::BinDisplay m_binDisplay;
};

#endif
