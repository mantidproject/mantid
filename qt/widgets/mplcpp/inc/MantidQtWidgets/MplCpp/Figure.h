#ifndef MPLCPP_FIGURE_H
#define MPLCPP_FIGURE_H
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

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @brief A C++ wrapper holding a matplotlib.Figure instance
 */
class MANTID_MPLCPP_DLL Figure : Python::InstanceHolder {
public:
  /// Creates a new matplotlib figure
  Figure();
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_FIGURE_H
