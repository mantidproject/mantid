// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/MplCpp/Colormap.h"
#include "MantidQtWidgets/MplCpp/Colors.h"
#include "MantidQtWidgets/MplCpp/DllConfig.h"

#include <optional>

#include <QRgb>
#include <QString>
#include <vector>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @brief A C++ wrapper around the matplotlib.cm.ScalarMappable
 * type to provide the capability to map an arbitrary data
 * value or array of values to an RGBA value(s) within a given colormap
 */
class MANTID_MPLCPP_DLL ScalarMappable : public Common::Python::InstanceHolder {
public:
  ScalarMappable(const NormalizeBase &norm, const Colormap &cmap);
  ScalarMappable(const NormalizeBase &norm, const QString &cmap);

  Colormap cmap() const;
  void setCmap(const Colormap &cmap);
  void setCmap(const QString &cmap);
  void setNorm(const NormalizeBase &norm);
  void setClim(std::optional<double> vmin = std::nullopt, std::optional<double> vmax = std::nullopt);
  QRgb toRGBA(double x, double alpha = 1.0) const;
  std::vector<QRgb> toRGBA(const std::vector<double> &x, double alpha = 1.0) const;
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
