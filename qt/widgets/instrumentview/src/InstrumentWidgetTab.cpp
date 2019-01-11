// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetTab.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"

namespace MantidQt {
namespace MantidWidgets {
InstrumentWidgetTab::InstrumentWidgetTab(InstrumentWidget *parent)
    : QFrame(parent), m_instrWidget(parent) {}

/**
 * Return a pointer to the projection surface.
 */
boost::shared_ptr<ProjectionSurface> InstrumentWidgetTab::getSurface() const {
  return m_instrWidget->getSurface();
}
} // namespace MantidWidgets
} // namespace MantidQt
