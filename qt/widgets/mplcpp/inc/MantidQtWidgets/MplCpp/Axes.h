#ifndef MPLCPP_AXES_H
#define MPLCPP_AXES_H
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
#include "MantidQtWidgets/MplCpp/Line2D.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

class MANTID_MPLCPP_DLL Axes : public Python::InstanceHolder {
public:
  Axes(Python::Object obj);

  /// @name Formatting
  /// @{
  void setXLabel(const char *label);
  void setYLabel(const char *label);
  void setTitle(const char *label);
  /// @}

  /// @name Plotting
  /// @{
  Line2D plot(std::vector<double> xdata, std::vector<double> ydata);
  /// @}
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_AXES_H
