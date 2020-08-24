// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/MplCpp/DllConfig.h"

#include <QColor>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * A static C++ wrapper around the matplotlib.colors.colorConverter instance.
 * It is used to translate colors of various formats to a QColor instance.
 */
class MANTID_MPLCPP_DLL ColorConverter : public Common::Python::InstanceHolder {
public:
  static QColor toRGB(const Common::Python::Object &colorSpec);
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
