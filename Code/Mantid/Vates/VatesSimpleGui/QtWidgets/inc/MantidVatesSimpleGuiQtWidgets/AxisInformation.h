#ifndef AXISINFORMATION_H_
#define AXISINFORMATION_H_

#include "MantidVatesSimpleGuiQtWidgets/WidgetDllOption.h"

#include <string>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
/**
 *
  This class provides a container for a given data axis information.

  @author Michael Reuter
  @date 24/05/2011

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_QTWIDGETS AxisInformation
{
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
  std::string getTitle() { return this->title; }

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
  void setTitle(std::string title) { this->title = title; }

private:
  std::string title; ///< The axis title (or label)
  double minimum; ///< The minimum extent of the axis
  double maximum; ///< The maximum extent of the axis
};

}
}
}

#endif // AXISINFORMATION_H_
