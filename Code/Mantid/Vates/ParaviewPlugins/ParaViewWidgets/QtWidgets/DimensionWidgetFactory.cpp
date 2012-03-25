#include "DimensionWidgetFactory.h"
#include "DimensionWidget.h"

/**
Constructor
@param binDisplay : Enum indicating what type of bin display should be used.
*/
DimensionWidgetFactory::DimensionWidgetFactory(Mantid::VATES::BinDisplay binDisplay) : m_binDisplay(binDisplay)
{
}

/**
Factory Method.
@return a new DimensionWidget
*/
Mantid::VATES::DimensionView* DimensionWidgetFactory::create() const
{
  DimensionWidget* widget = new DimensionWidget;
  widget->initalizeViewMode(m_binDisplay);
  return widget;
}