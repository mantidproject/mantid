#ifndef MPLCPP_COLORS_H
#define MPLCPP_COLORS_H
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

/**
 * @file Contains definitions of wrappers for types in
 * matplotlib.colors. These types provide the ability to
 * normalize data according to different scale types.
 * See https://matplotlib.org/2.2.3/api/colors_api.html
 */

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @brief C++ base class for Normalize types to allow a common interface
 * to distinguish from a general Python::InstanceHolder.
 */
class MANTID_MPLCPP_DLL NormalizeBase : public Python::InstanceHolder {
public:
  NormalizeBase(Python::Object obj);

  /// Return an appropriate object to determine the tick locations
  /// The default returns None indicating that matplotlib should autoselect it
  virtual Python::Object tickLocator() const { return Python::Object(); }
  /// Return an appropriate object to determine the text format type
  /// The default returns None indicating that matplotlib should autoselect it
  virtual Python::Object labelFormatter() const { return Python::Object(); }
};

/**
 * @brief The Normalize class provides a simple mapping of data in
 * the internal [vmin, vmax] to the interval [0, 1].
 * See
 * https://matplotlib.org/2.2.3/api/_as_gen/matplotlib.colors.Normalize.html#matplotlib.colors.Normalize
 */
class MANTID_MPLCPP_DLL Normalize : public NormalizeBase {
public:
  Normalize(double vmin, double vmax);
};

/**
 * @brief Map data values [vmin, vmax] onto a symmetrical log scale.
 * See
 * https://matplotlib.org/2.2.3/api/_as_gen/matplotlib.colors.SymLogNorm.html#matplotlib.colors.SymLogNorm
 */
class MANTID_MPLCPP_DLL SymLogNorm : public NormalizeBase {
public:
  static double DefaultLinearThreshold;
  static double DefaultLinearScale;

public:
  SymLogNorm(double linthresh, double linscale, double vmin, double vmax);

  Python::Object tickLocator() const override;
  Python::Object labelFormatter() const override;

private:
  // cache the linscale as it's not available publicly on the class
  double m_linscale;
};

/**
 * @brief Map data values [vmin, vmax] onto a power law scale.
 * See
 * https://matplotlib.org/2.2.3/api/_as_gen/matplotlib.colors.PowerNorm.html#matplotlib.colors.PowerNorm
 */
class MANTID_MPLCPP_DLL PowerNorm : public NormalizeBase {
public:
  PowerNorm(double gamma, double vmin, double vmax);
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
#endif // MPLCPP_COLORS_H
