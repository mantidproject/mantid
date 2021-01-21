// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/ImageInfoPresenter.h"
#include "MantidQtWidgets/Common/ImageInfoModelMD.h"
#include "MantidQtWidgets/Common/ImageInfoModelMatrixWS.h"

using Mantid::API::MatrixWorkspace;

namespace MantidQt {
namespace MantidWidgets {

/**
 * Constructor
 * @param view A pointer to a view object that displays the information
 */
ImageInfoPresenter::ImageInfoPresenter(IImageInfoWidget *view) : m_model(), m_view(std::move(view)) {
  m_view->setRowCount(2);
}

/**
 * @param x X position on an image of the workspace
 * @param y Y position on an image of the workspace
 * @param signal The signal value at the given position
 */
void ImageInfoPresenter::cursorAt(const double x, const double y, const double signal) {
  assert(m_model);
  m_view->showInfo(m_model->info(x, y, signal));
}

/**
 * Set a new workspace for the view to display
 * @param workspace A pointer to a new workspace
 */
void ImageInfoPresenter::setWorkspace(const Mantid::API::Workspace_sptr &workspace) {
  if (auto matrixWS = std::dynamic_pointer_cast<MatrixWorkspace>(workspace))
    m_model = std::make_unique<ImageInfoModelMatrixWS>(matrixWS);
  else {
    m_model = std::make_unique<ImageInfoModelMD>();
  }
  // Blank view
  cursorAt(ImageInfoModel::UnsetValue, ImageInfoModel::UnsetValue, ImageInfoModel::UnsetValue);
}

} // namespace MantidWidgets
} // namespace MantidQt
