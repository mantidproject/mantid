#include "DimensionWidgetFactory.h"
#include "DimensionWidget.h"

DimensionWidgetFactory::DimensionWidgetFactory(bool readOnlyLimits) : m_readOnlyLimits(readOnlyLimits)
{
}

Mantid::VATES::DimensionView* DimensionWidgetFactory::create() const
{
  return new DimensionWidget(m_readOnlyLimits);
}