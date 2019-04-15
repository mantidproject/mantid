// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_COLORCONVERTER_H
#define MPLCPP_COLORCONVERTER_H

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
class MANTID_MPLCPP_DLL ColorConverter : public Python::InstanceHolder {
public:
  static QColor toRGB(const Python::Object &colorSpec);
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_COLORCONVERTER_H
