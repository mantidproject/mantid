// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef EMODE_HANDLER_H
#define EMODE_HANDLER_H

#include "MantidQtWidgets/SpectrumViewer/DllOptionSV.h"
#include "MantidQtWidgets/SpectrumViewer/SpectrumDataSource.h"
#include "ui_SpectrumView.h"

/**
    @class EModeHandler

    This manages the instrument type combo box (emode) and E Fixed controls
    for the SpectrumView data viewer.

    @author Dennis Mikkelson
    @date   2012-10-12
 */

namespace MantidQt {
namespace SpectrumView {

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER EModeHandler {
public:
  /// Construct object to manage E Mode controls in the UI
  EModeHandler(Ui_SpectrumViewer *sv_ui);

  /// Get the E Mode to control units calculation, from the combo box
  int getEMode();

  /// Set the E Mode to control units calculation, in the combo box
  void setEMode(const int mode);

  /// Get the E Fixed value from the GUI
  double getEFixed();

  /// Set the E Fixed value in the GUI
  void setEFixed(const double efixed);

private:
  Ui_SpectrumViewer *m_svUI;
};

} // namespace SpectrumView
} // namespace MantidQt

#endif // EMODE_HANDLER_H
