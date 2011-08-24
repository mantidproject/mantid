#ifndef COLORUPDATER_H_
#define COLORUPDATER_H_

#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"

#include <QPair>

class pqColorMapModel;
class pqPipelineRepresentation;

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
/**
 *
  This class handles the application of requests from the ColorSelectionWidget.

  @author Michael Reuter
  @date 15/08/2011

  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS ColorUpdater
{
public:
  /// Object constructor
  ColorUpdater();
  /// Default destructor
  virtual ~ColorUpdater();

  /**
   * Set the color scale back to the original bounds.
   * @param repr the representation to auto color scale
   * @return the minimum and maximum values for the representation data range
   */
  QPair<double, double> autoScale(pqPipelineRepresentation *repr);
  /**
   * Set the requested color map on the data.
   * @param repr the representation to change the color map on
   * @param model the color map to use
   */
  void colorMapChange(pqPipelineRepresentation *repr,
                      const pqColorMapModel *model);
  /**
   * Set the data color scale range to the requested bounds.
   * @param repr the representation to change the color scale on
   * @param min the minimum bound for the color scale
   * @param max the maximum bound for the color scale
   */
  void colorScaleChange(pqPipelineRepresentation *repr, double min,
                        double max);
  /**
   * Set logarithmic color scaling on the data.
   * @param repr the representation to set logarithmic color scale
   * @param state flag to determine whether or not to use log color scaling
   */
  void logScale(pqPipelineRepresentation *repr, int state);
};

}
}
}

#endif // COLORUPDATER_H_
