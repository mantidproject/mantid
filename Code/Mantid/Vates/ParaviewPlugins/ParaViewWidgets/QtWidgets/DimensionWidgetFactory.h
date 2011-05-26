#ifndef _DIMENSION_WIDGET_FACTORY_H
#define _DIMENSION_WIDGET_FACTORY_H

#include "WidgetDllOption.h"
#include "MantidVatesAPI/DimensionViewFactory.h"

class EXPORT_OPT_MANTIDPARVIEW DimensionWidgetFactory  : public Mantid::VATES::DimensionViewFactory
{
public:
  DimensionWidgetFactory(bool readOnlyLimits);
  Mantid::VATES::DimensionView* create() const;
private:
  const bool m_readOnlyLimits;
};

#endif