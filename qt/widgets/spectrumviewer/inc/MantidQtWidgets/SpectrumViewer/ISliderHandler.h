// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef ISLIDER_HANDLER_H
#define ISLIDER_HANDLER_H

#include "MantidQtWidgets/SpectrumViewer/DllOptionSV.h"
#include "MantidQtWidgets/SpectrumViewer/SpectrumDataSource.h"
#include <QRect>

namespace MantidQt {
namespace SpectrumView {

/** An interface to the SliderHandler, which manages the horizontal and vertical
    scroll bars for the SpectrumView data viewer.
 */

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER ISliderHandler {

public:
  /// Construct object to manage image scrollbars from the specified UI
  ISliderHandler() {}
  virtual ~ISliderHandler() {}

  /// Configure the image scrollbars for the specified data and drawing area
  virtual void configureSliders(QRect drawArea,
                                SpectrumDataSource_sptr dataSource) = 0;

  /// Configure the horizontal scrollbar to cover the specified range
  virtual void configureHSlider(int nDataSteps, int nPixels) = 0;

  /// Return true if the image horizontal scrollbar is enabled.
  virtual bool hSliderOn() = 0;

  /// Return true if the image vertical scrollbar is enabled.
  virtual bool vSliderOn() = 0;

  /// Get the range of columns to display in the image.
  virtual void getHSliderInterval(int &xMin, int &xMax) = 0;

  /// Get the range of rows to display in the image.
  virtual void getVSliderInterval(int &yMin, int &yMax) = 0;
};

} // namespace SpectrumView
} // namespace MantidQt

#endif // ISLIDER_HANDLER_H
