// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef IRANGE_HANDLER_H
#define IRANGE_HANDLER_H

#include "MantidQtWidgets/SpectrumViewer/DllOptionSV.h"
#include "MantidQtWidgets/SpectrumViewer/SpectrumDataSource.h"

namespace MantidQt {
namespace SpectrumView {

/** An interface to the RangeHandler class, which manages the min, max and step
    range controls for the SpectrumView data viewer.
 */

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER IRangeHandler {

public:
  /// Construct object to manage min, max and step controls in the UI
  IRangeHandler() {}
  virtual ~IRangeHandler() {}

  /// Configure min, max and step controls for the specified data source
  virtual void configureRangeControls(SpectrumDataSource_sptr dataSource) = 0;

  /// Get the range of data to display in the image, from GUI controls
  virtual void getRange(double &min, double &max, double &step) = 0;
};

} // namespace SpectrumView
} // namespace MantidQt

#endif // IRANGE_HANDLER_H
