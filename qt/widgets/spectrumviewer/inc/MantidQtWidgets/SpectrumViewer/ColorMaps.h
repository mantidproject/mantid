#ifndef COLOR_MAPS_H
#define COLOR_MAPS_H

#include "MantidQtWidgets/SpectrumViewer/DllOptionSV.h"
#include <QColor>
#include <vector>

/**
    @class ColorMaps

       This class has static methods to construct some useful color scales
    and to build a lookup table to brighten an image, so low-level
    intensities become more visible

    @author Dennis Mikkelson
    @date   2012-04-03

    Copyright © 2012 ORNL, STFC Rutherford Appleton Laboratories

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

    Code Documentation is available at
                 <http://doxygen.mantidproject.org>
 */

namespace MantidQt {
namespace SpectrumView {

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER ColorMaps {

public:
  enum ColorScale {
    HEAT,
    GRAY,
    NEGATIVE_GRAY,
    GREEN_YELLOW,
    RAINBOW,
    OPTIMAL,
    MULTI,
    SPECTRUM
  };

  /// Get the specified color scale
  static std::vector<QRgb> GetColorMap(ColorScale name, size_t n_colors);

  /// Get look up table to brighten image
  static std::vector<double> GetIntensityMap(double control_s,
                                             size_t n_entries);

private:
  /// Fill out a color table by interpolating the given base RGB components
  static std::vector<QRgb> InterpolateColorScale(double base_red[],
                                                 double base_green[],
                                                 double base_blue[],
                                                 size_t n_base_colors,
                                                 size_t n_colors);
};

} // namespace SpectrumView
} // namespace MantidQt

#endif // COLOR_MAPS_H
