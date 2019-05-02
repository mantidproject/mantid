// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_PLOTTING_AXIS_H_
#define MANTIDQT_PLOTTING_AXIS_H_

namespace MantidQt {
namespace MantidWidgets {

/**
 * Defines an type to indentify an axis on a plot.
 */
enum class AxisID : int { YLeft, YRight, XBottom, XTop };

/**
 * Return an integer from an AxisID that represents the same axis in Qwt
 * @param id An AxisID type
 * @return An integer defining the same axis
 */
inline constexpr int toQwtAxis(const AxisID id) { return static_cast<int>(id); }

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQT_PLOTTING_AXIS_H_
