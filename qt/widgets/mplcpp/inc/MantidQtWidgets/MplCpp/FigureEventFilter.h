// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/MplCpp/DllConfig.h"

#include <QEvent>
#include <QObject>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @brief Provides a QObject that can be installed to filter out unwanted
 * QEvent's for a C++ Matplotlib Figure. For example, we do not want to
 * process event's which attempt to do a negative resizing of a Matplotlib
 * Figure because this causes an exception.
 */
class MANTID_MPLCPP_DLL FigureEventFilter : public QObject {
  Q_OBJECT

protected:
  bool eventFilter(QObject *obj, QEvent *ev) override;
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
