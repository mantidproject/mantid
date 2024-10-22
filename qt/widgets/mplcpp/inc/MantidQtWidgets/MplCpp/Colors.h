// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/MplCpp/DllConfig.h"

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
class MANTID_MPLCPP_DLL NormalizeBase : public Common::Python::InstanceHolder {
public:
  /// Autoscale the limits to vmin, vmax, clamping any invalid values
  virtual std::tuple<double, double> autoscale(std::tuple<double, double> clim);

  /// Return an appropriate object to determine the tick locations
  /// The default returns None indicating that matplotlib should autoselect it
  virtual Common::Python::Object tickLocator() const { return Common::Python::Object(); }
  /// Return an appropriate object to determine the text format type
  /// The default returns None indicating that matplotlib should autoselect it
  virtual Common::Python::Object labelFormatter() const { return Common::Python::Object(); }

protected:
  // Only to be called by derived classes. They should ensure
  // this object is of the correct type
  NormalizeBase(Common::Python::Object pyobj);
};

/**
 * @brief The Normalize class provides a simple mapping of data in
 * the internal [vmin, vmax] to the interval [0, 1].
 * See
 * https://matplotlib.org/2.2.3/api/_as_gen/matplotlib.colors.Normalize.html#matplotlib.colors.Normalize
 */
class MANTID_MPLCPP_DLL Normalize : public NormalizeBase {
public:
  Normalize();
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
  SymLogNorm(double linthresh, double linscale);
  SymLogNorm(double linthresh, double linscale, double vmin, double vmax);

  Common::Python::Object tickLocator() const override;
  Common::Python::Object labelFormatter() const override;

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
  PowerNorm(double gamma);
  PowerNorm(double gamma, double vmin, double vmax);
  virtual std::tuple<double, double> autoscale(std::tuple<double, double> clim) override;
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
