// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef REF_MATRIX_WS_IMAGE_VIEW_H
#define REF_MATRIX_WS_IMAGE_VIEW_H

#include "DllOption.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/RefDetectorView/RefImageView.h"

/**
    @class RefMatrixWSImageView

    This is the top level class for showing a matrix workspace
    using an ImageViewer.

    @author Dennis Mikkelson
    @date   2012-05-08
 */

namespace MantidQt {
namespace RefDetectorViewer {
class RefIVConnections;

class EXPORT_OPT_MANTIDQT_REFDETECTORVIEWER RefMatrixWSImageView {

public:
  /// Construct an image viewer for the specifed MatrixWorkspace
  RefMatrixWSImageView(Mantid::API::MatrixWorkspace_sptr /*mat_ws*/);

  RefMatrixWSImageView(QString wpsName, int peakMin, int peakMax, int backMin,
                       int backMax, int tofMin, int tofMax);

  RefIVConnections *getConnections();

  ~RefMatrixWSImageView();

private:
  RefImageView *m_imageView;
};

} // namespace RefDetectorViewer
} // namespace MantidQt

#endif // REF_MATRIX_WS_IMAGE_VIEW_H
