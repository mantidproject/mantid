// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_COLORMAP_H
#define MPLCPP_COLORMAP_H

#include "MantidQtWidgets/MplCpp/DllConfig.h"
#pragma push_macro("slots")
#undef slots
#include "MantidQtWidgets/Common/Python/Object.h"
#pragma pop_macro("slots")
#include <QString>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @brief Defines a C++ wrapper for the matplotlib.cm.Colormap
 * class
 */
class MANTID_MPLCPP_DLL Colormap : public Common::Python::InstanceHolder {
public:
  Colormap(Common::Python::Object obj);
};

/// Return the matplotlib.cm module
MANTID_MPLCPP_DLL Common::Python::Object cmModule();

/// Check if the named colormap if it exists
MANTID_MPLCPP_DLL bool cmapExists(const QString &name);

/// Return the named colormap if it exists
MANTID_MPLCPP_DLL Colormap getCMap(const QString &name);

/// Return the named colormap if it exists
MANTID_MPLCPP_DLL QString defaultCMapName();

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_COLORMAP_H
