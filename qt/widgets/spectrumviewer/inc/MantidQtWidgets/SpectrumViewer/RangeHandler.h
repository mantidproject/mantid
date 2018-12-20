// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef RANGE_HANDLER_H
#define RANGE_HANDLER_H

#include "MantidQtWidgets/SpectrumViewer/DllOptionSV.h"
#include "MantidQtWidgets/SpectrumViewer/IRangeHandler.h"
#include "MantidQtWidgets/SpectrumViewer/SpectrumDataSource.h"
#include "ui_SpectrumView.h"

/**
    @class RangeHandler

    This manages the min, max and step range controls for the SpectrumView
    data viewer.

    @author Dennis Mikkelson
    @date   2012-04-25
 */

namespace MantidQt {
namespace SpectrumView {

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER RangeHandler : public IRangeHandler {
public:
  /// Construct object to manage min, max and step controls in the UI
  RangeHandler(Ui_SpectrumViewer *svUI);

  /// Configure min, max and step controls for the specified data source
  void configureRangeControls(SpectrumDataSource_sptr dataSource) override;

  /// Get the range of data to display in the image, from GUI controls
  void getRange(double &min, double &max, double &step) override;

  /// Set the values displayed in the GUI controls
  void setRange(double min, double max, double step);

private:
  Ui_SpectrumViewer *m_svUI;
  double m_totalMinX;
  double m_totalMaxX;
  size_t m_totalNSteps;
};

} // namespace SpectrumView
} // namespace MantidQt

#endif // RANGE_HANDLER_H
