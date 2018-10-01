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
#include "MantidQtWidgets/MplCpp/Axes.h"
#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include "MantidQtWidgets/MplCpp/Python/Object.h"
#include "MantidQtWidgets/MplCpp/ScalarMappable.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * A thin C++ wrapper to create a new matplotlib figure
 */
class MANTID_MPLCPP_DLL Figure : public Python::InstanceHolder {
public:
  Figure(Python::Object obj);
  Figure(bool tightLayout = true);

  /**
   * @brief Access (and create if necessar) the active Axes
   * @return An instance of Axes attached to the figure
   */
  inline Axes gca() const { return Axes{pyobj().attr("gca")()}; }
  /**
   * @param index The index of the axes to return
   * @return The axes instance
   */
  inline Axes axes(size_t index) const {
    return Axes{pyobj().attr("axes")[index]};
  }

  Axes addAxes(double left, double bottom, double width, double height);
  Axes addSubPlot(int subplotspec);
  Python::Object colorbar(const ScalarMappable &mappable, const Axes &cax,
                          const Python::Object &ticks = Python::Object(),
                          const Python::Object &format = Python::Object());
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_FIGURE_H
