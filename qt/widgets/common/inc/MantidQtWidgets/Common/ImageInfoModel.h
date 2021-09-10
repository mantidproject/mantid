// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"

#include "MantidAPI/Workspace.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include <QString>
#include <vector>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON ImageInfoModel {
public:
  class ImageInfo {
  public:
    using StringItems = std::vector<QString>;
    ImageInfo(StringItems names);

    inline bool empty() const noexcept { return m_names.empty(); }
    inline int size() const noexcept { return static_cast<int>(m_names.size()); }
    inline const QString &name(int index) const noexcept { return m_names[index]; }
    inline const QString &value(int index) const noexcept { return m_values[index]; }
    inline void setValue(int index, const QString &value) noexcept { m_values[index] = value; }
    StringItems m_names;
    StringItems m_values;
  };

  /// Default float precision
  static constexpr auto FourDigitPrecision = 4;
  /// Default float format
  static constexpr auto DecimalFormat = 'f';
  /// Value to indicate that a MissingValue should be shown
  static constexpr auto UnsetValue = std::numeric_limits<double>::max();
  /// MissingValue identifier
  static inline const QString MissingValue = QString("-");

  static inline const QString defaultFormat(const double x) {
    return QString::number(x, ImageInfoModel::DecimalFormat, ImageInfoModel::FourDigitPrecision);
  }

  static inline const QString defaultFormat(const int x) { return QString::number(x); }

public:
  virtual ~ImageInfoModel() = default;

  /** Creates information about the point at the given coordinates in the
  workspace.
  @param x: x data coordinate
  @param y: y data coordinate
  @param signal: the signal value at x, y
  @return An ImageInfo object
  */
  virtual ImageInfoModel::ImageInfo info(const double x, const double y, const double signal) const = 0;
};

} // namespace MantidWidgets
} // namespace MantidQt
