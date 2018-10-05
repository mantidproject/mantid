// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MATRIX_WS_SPECTRUM_VIEW_H
#define MATRIX_WS_SPECTRUM_VIEW_H

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtSpectrumViewer/DllOptionSV.h"

#include "MantidQtSpectrumViewer/SpectrumView.h"

/**
    @class MatrixWSDataSource

    This is the top level class for showing a matrix workspace
    using an SpectrumViewer.

    @author Dennis Mikkelson
    @date   2012-05-08
 */

namespace MantidQt {
namespace SpectrumView {

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER MatrixWSSpectrumView {
public:
  /// Construct a spectrum viewer for the specifed MatrixWorkspace
  MatrixWSSpectrumView(Mantid::API::MatrixWorkspace_const_sptr mat_ws);

  ~MatrixWSSpectrumView();

private:
  SpectrumView *spectrum_view;
};

} // namespace SpectrumView
} // namespace MantidQt

#endif // MATRIX_WS_SPECTRUM_VIEW_H
