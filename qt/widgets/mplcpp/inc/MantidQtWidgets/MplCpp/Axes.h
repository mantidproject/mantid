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

#include <QString>

#include <tuple>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

class MANTID_MPLCPP_DLL Axes : public Python::InstanceHolder {
public:
  explicit Axes(Python::Object obj);

  /// @name Formatting
  /// @{
  void setXLabel(const char *label);
  void setYLabel(const char *label);
  void setTitle(const char *label);
  /// @}

  /// @name Drawing
  /// @{
  Line2D plot(std::vector<double> xdata, std::vector<double> ydata,
              const char *format = "b-");
  Artist text(double x, double y, QString text,
              const char *horizontalAlignment);
  /// @}

  ///@name Scales
  /// @{
  void setXScale(const char *value);
  QString getXScale() const;
  void setYScale(const char *value);
  QString getYScale() const;
  std::tuple<double, double> getXLim() const;
  std::tuple<double, double> getYLim() const;
  void relim(bool visibleOnly = false);
  void autoscale(bool enable);
  void autoscaleView(bool scaleX = true, bool scaleY = true);
  void autoscaleView(bool tight, bool scaleX = true, bool scaleY = true);
  /// @}
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_AXES_H
