#ifndef MPLCPP_COLORMAP_H
#define MPLCPP_COLORMAP_H
/*
  Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/
#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include "MantidQtWidgets/MplCpp/Python/Object.h"
#include <QString>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @brief Defines a wrapper
 */
class MANTID_MPLCPP_DLL Colormap : public Python::InstanceHolder {
public:
  Colormap(Python::Object obj);
};

/// Return the named colormap if it exists
MANTID_MPLCPP_DLL Colormap getCMap(const QString &name);

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_COLORMAP_H
