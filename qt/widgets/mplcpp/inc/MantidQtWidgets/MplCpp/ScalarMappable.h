#ifndef MPLCPP_SCALARMAPPABLE_H
#define MPLCPP_SCALARMAPPABLE_H
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
#include "MantidQtWidgets/MplCpp/Colormap.h"
#include "MantidQtWidgets/MplCpp/Colors.h"
#include "MantidQtWidgets/MplCpp/DllConfig.h"

#include <boost/optional/optional.hpp>

#include <QRgb>
#include <QString>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @brief A C++ wrapper around the matplotlib.cm.ScalarMappable
 * type to provide the capability to map an arbitrary data
 * value to an RGBA value within a given colormap
 */
class MANTID_MPLCPP_DLL ScalarMappable : public Python::InstanceHolder {
public:
  ScalarMappable(const NormalizeBase &norm, const Colormap &cmap);
  ScalarMappable(const NormalizeBase &norm, const QString &cmap);

  void setCmap(const Colormap &cmap);
  void setCmap(const QString &cmap);
  void setNorm(const NormalizeBase &norm);
  void setClim(boost::optional<double> vmin = boost::none,
               boost::optional<double> vmax = boost::none);
  QRgb toRGBA(double x, double alpha = 1.0) const;
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_SCALARMAPPABLE_H
