// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
