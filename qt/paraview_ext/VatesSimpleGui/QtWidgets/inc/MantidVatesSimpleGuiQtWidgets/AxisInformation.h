// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef AXISINFORMATION_H_
#define AXISINFORMATION_H_

#include "MantidVatesSimpleGuiQtWidgets/WidgetDllOption.h"

#include <string>

namespace Mantid {
namespace Vates {
namespace SimpleGui {
/**
 *
  This class provides a container for a given data axis information.

  @author Michael Reuter
  @date 24/05/2011
 */
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_QTWIDGETS AxisInformation {
public:
  /// Default constructor.
  AxisInformation();
  /// Default destructor.
  virtual ~AxisInformation() {}

  /**
   * Get the maximum extent of the associated axis.
   * @return the maximum axis extent
   */
  double getMaximum() { return this->maximum; }
  /**
   * Get the minimum extent of the associated axis.
   * @return the minimum axis extent
   */
  double getMinimum() { return this->minimum; }
  /**
   * Get the title of the associated axis.
   * @return the axis title
   */
  const std::string &getTitle() { return this->title; }

  /**
   * Set the maximum extent of the associated axis.
   * @param max the maximum axis extent to apply
   */
  void setMaximum(double max) { this->maximum = max; }
  /**
   * Set the minimum extent of the associated axis.
   * @param min the minimum axis extent to apply
   */
  void setMinimum(double min) { this->minimum = min; }
  /**
   * Set the title of the associated axis.
   * @param title the axis title to apply
   */
  void setTitle(const std::string &title) { this->title = title; }

private:
  double minimum;    ///< The minimum extent of the axis
  double maximum;    ///< The maximum extent of the axis
  std::string title; ///< The axis title (or label)
};
} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid

#endif // AXISINFORMATION_H_
