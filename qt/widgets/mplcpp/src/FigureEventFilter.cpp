// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/FigureEventFilter.h"

#include <QResizeEvent>

namespace MantidQt::Widgets::MplCpp {

bool FigureEventFilter::eventFilter(QObject *obj, QEvent *ev) {
  if (ev->type() == QEvent::Resize) {
    auto &&resizeEvent = static_cast<QResizeEvent *>(ev);
    if (resizeEvent->size().isEmpty()) {
      // The resize is negative or zero and will cause an exception, so stop processing the event
      return true;
    }
  }
  return QObject::eventFilter(obj, ev);
}

} // namespace MantidQt::Widgets::MplCpp
