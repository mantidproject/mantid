// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef REF_RANGE_HANDLER_H
#define REF_RANGE_HANDLER_H

#include "DllOption.h"
#include "MantidQtWidgets/SpectrumViewer/IRangeHandler.h"
#include "MantidQtWidgets/SpectrumViewer/SpectrumDataSource.h"
#include "ui_RefImageView.h"

/**
    @class RangeHandler

    This manages the min, max and step range controls for the SpectrumView
    data viewer.

    @author Dennis Mikkelson
    @date   2012-04-25
 */

namespace MantidQt {
namespace RefDetectorViewer {

class EXPORT_OPT_MANTIDQT_REFDETECTORVIEWER RefRangeHandler
    : public SpectrumView::IRangeHandler {
public:
  /// Construct object to manage min, max and step controls in the UI
  RefRangeHandler(Ui_RefImageViewer *ivUI);

  /// Configure min, max and step controls for the specified data source
  void configureRangeControls(
      SpectrumView::SpectrumDataSource_sptr dataSource) override;

  /// Get the range of data to display in the image, from GUI controls
  void getRange(double &min, double &max, double &step) override;

  /// Set the values displayed in the GUI controls
  void setRange(double min, double max, double step, char type);

private:
  Ui_RefImageViewer *m_ivUI;

  double m_totalMinX;
  double m_totalMaxX;
  double m_totalMinY;
  double m_totalMaxY;
  size_t m_totalNSteps;
};

} // namespace RefDetectorViewer
} // namespace MantidQt

#endif // REF_RANGE_HANDLER_H
