#ifndef AUTOSCALERANGEGENERATOR_H
#define AUTOSCALERANGEGENERATOR_H

/**
    Generates information for the color scale, e.g. minimum level, maximum level, 
    log scale.

    @date 18/11/2014

    Copyright &copy; 2007-11 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>

*/
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"
#include <qlist.h>

class pqPipelineSource;

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

typedef struct VsiColorScale
{
  double maxValue;

  double minValue;

  bool useLogScale;
} VsiColorScale;

class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS  AutoScaleRangeGenerator
{
  public:
    AutoScaleRangeGenerator();

    virtual ~AutoScaleRangeGenerator() {};

    /// Creates a color scale entity.
    VsiColorScale getColorScale();

    /// Enum for different modes
    typedef enum COLORSCALEMODE{STANDARD, TECHNIQUEDEPENDENT, OFFSET} COLORSCALEMODE;

  private:
    /// Selected color scale mode.
    COLORSCALEMODE mode;

    /// Default value for the color scale
    double defaultValue;

    /// Get the color scale for the standard selection.
    VsiColorScale getStandardColorScale();

    /// Get all ParaView sources from the active server.
    QList<pqPipelineSource *> getAllPVSources();

    /// Make sure that the color scale is valid.
    void sanityCheck(VsiColorScale& colorscale);

  /**
   * Extract the min and max values of a source. If we are dealing with a filter which does not
   * have the information then look upstream for the information
   * @param source A pointer to a source
   * @param minValueBuffer A reference to a min value.
   * @param maxValueBuffer A reference to a max value.
   */
  void AutoScaleRangeGenerator::setMinBufferAndMaxBuffer(pqPipelineSource* source, double& minValue, double& maxValue);

};

}
}
}
#endif

